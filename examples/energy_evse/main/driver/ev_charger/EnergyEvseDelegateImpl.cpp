// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#ifdef UNIT_TEST
#include "helpers.h"
#include "get_readable_time.h"
#include "chip_support.h"
#include "chip_im_status.h"
#include "chip_logging.h"
#include "chip_system_layer.h"
#include "esp_matter_evse.h"
#include "matterManager.h"
#include "chargerManager.h"
#include "EnergyTimeUtils.h"
#include "EnergyEvseTargetsStore.h"
#include "EnergyEvseDelegateImpl.h"
#include "app/EventLogging.h"
#include "app/MessageDef/StatusIB.h"

#include <algorithm>
#include <cstddef>
#include <string>

#undef CHIP_ERROR_FORMAT
#define CHIP_ERROR_FORMAT "%d"
#define CHIP_ERR_FMT(e) static_cast<int>((e).AsInteger())
#else
#include <app/EventLogging.h>
#include <cstddef>
#include <string>

#include "helpers.h"
#include "get_readable_time.h"

#include "matterManager.h"
#include "chargerManager.h"

#include "EnergyTimeUtils.h"
#include "EnergyEvseTargetsStore.h"

#include "EnergyEvseDelegateImpl.h"
#define CHIP_ERR_FMT(e) (e).Format()
#endif

using namespace chip;
using namespace chip::app;
using namespace chip::app::DataModel;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::EnergyEvse;
using namespace chip::app::Clusters::EnergyEvse::Attributes;

using namespace CT::Charger;

using chip::app::LogEvent;
using chip::app::StatusIB;

using std::min;

namespace {

bool TargetSkippedAsPast(uint16_t targetMinutesPastMidnight, uint16_t minutesPastMidnightNow_m, bool allowTargetsInPast)
{
    return !allowTargetsInPast && targetMinutesPastMidnight < minutesPastMidnightNow_m;
}

void TakeEarlierTarget(const EnergyEvse::Structs::ChargingTargetStruct::Type &chargingTarget,
                       uint16_t &minTimeToTarget_m, bool &bFound, uint16_t &targetTimeMinutesPastMidnight_m,
                       DataModel::Nullable<Percent> &targetSoC, DataModel::Nullable<int64_t> &addedEnergy_mWh)
{
    if (chargingTarget.targetTimeMinutesPastMidnight >= minTimeToTarget_m) {
        return;
    }

    minTimeToTarget_m = chargingTarget.targetTimeMinutesPastMidnight;
    bFound = true;
    targetTimeMinutesPastMidnight_m = chargingTarget.targetTimeMinutesPastMidnight;

    if (chargingTarget.targetSoC.HasValue()) {
        targetSoC.SetNonNull(chargingTarget.targetSoC.Value());
    } else {
        targetSoC.SetNull();
    }

    if (chargingTarget.addedEnergy.HasValue()) {
        addedEnergy_mWh.SetNonNull(chargingTarget.addedEnergy.Value());
    } else {
        addedEnergy_mWh.SetNull();
    }
}

} // namespace

EnergyEvseDelegate::EnergyEvseDelegate()
    : Delegate()
{
    SetEndpointId(0);
}

void EnergyEvseDelegate::SetupDelegate(EndpointId id)
{
    SetEndpointId(id);
    mSession.SetEndpointId(id);

    AddCustomAttributes();

    if constexpr (kSupportDischargingV2x) {
        AddCustomFeatures(BitMask<EnergyEvse::Feature, uint32_t>(
            EnergyEvse::Feature::kChargingPreferences, EnergyEvse::Feature::kRfid, EnergyEvse::Feature::kSoCReporting,
            EnergyEvse::Feature::kV2x));
    } else {
        AddCustomFeatures(BitMask<EnergyEvse::Feature, uint32_t>(
            EnergyEvse::Feature::kChargingPreferences, EnergyEvse::Feature::kRfid, EnergyEvse::Feature::kSoCReporting));
    }
}

void EnergyEvseDelegate::AddCustomAttributes()
{
    // add attributes that is not supported by the esp-matter libraries yet
    esp_matter::cluster_t *cluster = esp_matter::cluster::get(GetEndpointId(), Clusters::EnergyEvse::Id);

    using namespace esp_matter::cluster;
    energy_evse::attribute::create_user_maximum_charge_current(cluster,
                                                               ChargerManager::Controller().config.currentLimit_HW);
}

void EnergyEvseDelegate::AddCustomFeatures(
#ifndef UNIT_TEST
    Feature aFeature)
#else
    BitMask<Feature, uint32_t> aFeature)
#endif
{
#ifndef UNIT_TEST
    mFeature.Set(aFeature);
#else
    mFeature = aFeature;
#endif

    esp_matter::cluster_t *cluster = esp_matter::cluster::get(GetEndpointId(), Clusters::EnergyEvse::Id);

    using namespace esp_matter::cluster::energy_evse;
    if (mFeature.Has(Feature::kChargingPreferences)) {
        feature::charging_preferences::add(cluster);
    }

    if (mFeature.Has(Feature::kRfid)) {
        feature::rfid::add(cluster);
    }

    if (mFeature.Has(Feature::kSoCReporting)) {
        feature::soc_reporting::add(cluster);
    }

    if (mFeature.Has(Feature::kV2x)) {
        feature::v2x::add(cluster);
    }
}

void EnergyEvseDelegate::LateSetupAfterMatter()
{
    PRINTF_DEBUG();
    // first check if charging is enabled
    DataModel::Nullable<uint32_t> enabledUntilTime = GetChargingEnabledUntil();
    if (enabledUntilTime.IsNull()) {
        SetSupplyState(SupplyStateEnum::kChargingEnabled);
        mMaximumChargingCurrentLimitFromCommand = GetUserMaximumChargeCurrent();

        ComputeMaxChargeCurrentLimit();
    }

    mEvseTargetsDelegate.Init(&Server::GetInstance().GetPersistentStorage());
    PRINTF_DEBUG("Set targets GetPersistentStorage");

    HwRegisterEvseCallbackHandler(ApplicationCallbackHandler, reinterpret_cast<intptr_t>(this));
}

[[maybe_unused]] static const char *GetDayOfWeekStr(BitMask<EnergyEvse::TargetDayOfWeekBitmap> dayMask)
{
    if (dayMask.Has(EnergyEvse::TargetDayOfWeekBitmap::kMonday))
        return "Monday";
    if (dayMask.Has(EnergyEvse::TargetDayOfWeekBitmap::kTuesday))
        return "Tuesday";
    if (dayMask.Has(EnergyEvse::TargetDayOfWeekBitmap::kWednesday))
        return "Wednesday";
    if (dayMask.Has(EnergyEvse::TargetDayOfWeekBitmap::kThursday))
        return "Thursday";
    if (dayMask.Has(EnergyEvse::TargetDayOfWeekBitmap::kFriday))
        return "Friday";
    if (dayMask.Has(EnergyEvse::TargetDayOfWeekBitmap::kSaturday))
        return "Saturday";
    if (dayMask.Has(EnergyEvse::TargetDayOfWeekBitmap::kSunday))
        return "Sunday";
    return "Unknown";
}

Status EnergyEvseDelegate::Disable()
{
    PRINTF_DEBUG("EnergyEvseDelegate::Disable()");

    DataModel::Nullable<uint32_t> disableTime(0);
    /* update ChargingEnabledUntil & DischargingEnabledUntil to show 0 */
    SetChargingEnabledUntil(disableTime);
    SetDischargingEnabledUntil(disableTime);

    /* update MinimumChargeCurrent & MaximumChargeCurrent to 0 */
    SetMinimumChargeCurrent(0);

    mMaximumChargingCurrentLimitFromCommand = 0;
    ComputeMaxChargeCurrentLimit();

    /* update MaximumDischargeCurrent to 0 */
    SetMaximumDischargeCurrent(0);

    return HandleDisabledEvent();
}

Status EnergyEvseDelegate::EnableCharging(const DataModel::Nullable<uint32_t> &chargingEnabledUntil,
                                          const int64_t &minimumChargeCurrent, const int64_t &maximumChargeCurrent)
{
    PRINTF_DEBUG("EnableCharging CMD received: chargingEnabledUntil=%s, minCC=%lld, maxCC=%lld",
                 chargingEnabledUntil.IsNull() ? "null" : std::to_string(chargingEnabledUntil.Value()).c_str(),
                 minimumChargeCurrent, maximumChargeCurrent);

    PRINTF_DEBUG("EnergyEvseDelegate::EnableCharging()");

    if (maximumChargeCurrent < kMinimumChargeCurrent) {
        PRINTF_DEBUG("Maximum Current outside limits");
        return Status::ConstraintError;
    }

    if (minimumChargeCurrent < kMinimumChargeCurrent) {
        PRINTF_DEBUG("Maximum Current outside limits");
        return Status::ConstraintError;
    }

    if (minimumChargeCurrent > maximumChargeCurrent) {
        PRINTF_DEBUG("Minium Current > Maximum Current!");
        return Status::ConstraintError;
    }

    if (chargingEnabledUntil.IsNull()) {
        /* Charging enabled indefinitely */
        PRINTF_DEBUG("Charging enabled indefinitely");
    } else {
        /* check chargingEnabledUntil is in the future */
        PRINTF_DEBUG("Charging enabled until: %lu", static_cast<long unsigned int>(chargingEnabledUntil.Value()));
    }
    SetChargingEnabledUntil(chargingEnabledUntil);

    /* If it looks ok, store the min & max charging current */
    mMaximumChargingCurrentLimitFromCommand = maximumChargeCurrent;
    SetMinimumChargeCurrent(minimumChargeCurrent);
    // TODO persist these to KVS

    ComputeMaxChargeCurrentLimit();

    return HandleChargingEnabledEvent();
}

Status EnergyEvseDelegate::EnableDischarging(const DataModel::Nullable<uint32_t> &enableDischargeTime,
                                             const int64_t &maximumDischargeCurrent)
{
    if constexpr (kSupportDischargingV2x) {
        return HandleChargingEnabledEvent();
    } else {
        return Status::UnsupportedAttribute;
    }
}

static void FakeDiagnosticProcessEnd([[maybe_unused]] System::Layer *systemLayer, void *appState)
{
    // chip::System::TimerCompleteCallback passes opaque context; registered with `this`.
    auto dg = static_cast<EnergyEvseDelegate *>(appState);

    dg->SetSupplyState(SupplyStateEnum::kDisabled);
}
Status EnergyEvseDelegate::StartDiagnostics()
{
    /* For EVSE manufacturers to customize */
    PRINTF_DEBUG("EnergyEvseDelegate::StartDiagnostics()");

    if (mSupplyState != SupplyStateEnum::kDisabled) {
        PRINTF_DEBUG("EVSE: cannot be put into diagnostics mode if it is not Disabled!");
        return Status::Failure;
    }

#if 1
    if (mSupplyState == SupplyStateEnum::kDisabledDiagnostics) {
        PRINTF_DEBUG("EVSE: already in diagnostics mode");
        return Status::Success;
    }

    /* start a time to pretend we are doing something and then send it back to normal or last supply state*/
    DeviceLayer::SystemLayer().StartTimer(System::Clock::Seconds32(30), FakeDiagnosticProcessEnd, this);
#endif

    // Update the SupplyState - this will automatically callback the Application StateChanged callback
    SetSupplyState(SupplyStateEnum::kDisabledDiagnostics);

    return Status::Success;
}

Status EnergyEvseDelegate::SetTargets(
    const DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType> &chargingTargetSchedules)
{
    PRINTF_DEBUG("EnergyEvseDelegate::SetTargets()");

    EvseTargetsDelegate *targets = GetEvseTargetsDelegate();
    VerifyOrReturnError(targets != nullptr, Status::Failure);

    CHIP_ERROR err = targets->SetTargets(chargingTargetSchedules);
    VerifyOrReturnError(err == CHIP_NO_ERROR, StatusIB(err).mStatus);

    /* The Application needs to be told that the Targets have been updated
     * so it can potentially re-optimize the charging start time etc
     */
    NotifyApplicationChargingPreferencesChange();

    return Status::Success;
}

Status EnergyEvseDelegate::LoadTargets()
{
    PRINTF_DEBUG("EnergyEvseDelegate::LoadTargets()");

    EvseTargetsDelegate *targets = GetEvseTargetsDelegate();
    VerifyOrReturnError(targets != nullptr, StatusIB(CHIP_ERROR_UNINITIALIZED).mStatus);

    CHIP_ERROR err = targets->LoadTargets();
    VerifyOrReturnError(err == CHIP_NO_ERROR, StatusIB(err).mStatus);

    return Status::Success;
}

Status EnergyEvseDelegate::GetTargets(
    DataModel::List<const Structs::ChargingTargetScheduleStruct::Type> &chargingTargetSchedules)
{
    PRINTF_DEBUG("EnergyEvseDelegate::GetTargets()");

    EvseTargetsDelegate *targets = GetEvseTargetsDelegate();
    VerifyOrReturnError(targets != nullptr, StatusIB(CHIP_ERROR_UNINITIALIZED).mStatus);

    chargingTargetSchedules = targets->GetTargets();

    return Status::Success;
}

Status EnergyEvseDelegate::ClearTargets()
{
    PRINTF_DEBUG("EnergyEvseDelegate::ClearTargets()");

    CHIP_ERROR err;

    EvseTargetsDelegate *targets = GetEvseTargetsDelegate();
    if (targets == nullptr) {
        return StatusIB(CHIP_ERROR_UNINITIALIZED).mStatus;
    }

    err = targets->ClearTargets();
    if (err != CHIP_NO_ERROR) {
        PRINTF_DEBUG("Failed to clear Evse targets: %" CHIP_ERROR_FORMAT, CHIP_ERR_FMT(err));
        return Status::Failure;
    }

    /* The Application needs to be told that the Targets have been deleted
     * so it can potentially re-optimize the charging start time etc
     */
    NotifyApplicationChargingPreferencesChange();

    return Status::Success;
}

StateEnum EnergyEvseDelegate::GetState()
{
    return mState;
}
CHIP_ERROR EnergyEvseDelegate::SetState(StateEnum newValue)
{
    StateEnum oldValue = mState;
    if (newValue >= StateEnum::kUnknownEnumValue) {
        return CHIP_IM_GLOBAL_STATUS(ConstraintError);
    }

    mState = newValue;
    if (oldValue != mState) {
        PRINTF_DEBUG("State updated to %d", static_cast<int>(mState));
        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, State::Id);
        NotifyApplicationStateChange();
    }

    return CHIP_NO_ERROR;
}

SupplyStateEnum EnergyEvseDelegate::GetSupplyState()
{
    return mSupplyState;
}
CHIP_ERROR EnergyEvseDelegate::SetSupplyState(SupplyStateEnum newValue)
{
    SupplyStateEnum oldValue = mSupplyState;

    if (newValue >= SupplyStateEnum::kUnknownEnumValue) {
        return CHIP_IM_GLOBAL_STATUS(ConstraintError);
    }

    mSupplyState = newValue;
    if (oldValue != mSupplyState) {
        PRINTF_DEBUG("SupplyState updated to %d", static_cast<int>(mSupplyState));
        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, SupplyState::Id);
        NotifyApplicationStateChange();
    }
    return CHIP_NO_ERROR;
}

FaultStateEnum EnergyEvseDelegate::GetFaultState()
{
    return mFaultState;
}
CHIP_ERROR EnergyEvseDelegate::SetFaultState(FaultStateEnum newValue)
{
    FaultStateEnum oldValue = mFaultState;

    if (newValue >= FaultStateEnum::kUnknownEnumValue) {
        return CHIP_IM_GLOBAL_STATUS(ConstraintError);
    }

    mFaultState = newValue;
    if (oldValue != mFaultState) {
        PRINTF_DEBUG("FaultState updated to %d", static_cast<int>(mFaultState));
        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, FaultState::Id);
    }
    return CHIP_NO_ERROR;
}

DataModel::Nullable<uint32_t> EnergyEvseDelegate::GetChargingEnabledUntil()
{
    return mChargingEnabledUntil;
}
CHIP_ERROR EnergyEvseDelegate::SetChargingEnabledUntil(const DataModel::Nullable<uint32_t> &newValue)
{
    DataModel::Nullable<uint32_t> oldValue = mChargingEnabledUntil;

    mChargingEnabledUntil = newValue;
    if (oldValue != newValue) {
        if (newValue.IsNull()) {
            PRINTF_DEBUG("ChargingEnabledUntil updated to Null");
        } else {
            PRINTF_DEBUG("ChargingEnabledUntil updated to %lu",
                         static_cast<unsigned long int>(mChargingEnabledUntil.Value()));
        }

        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, ChargingEnabledUntil::Id);
    }

    return CHIP_NO_ERROR;
}

DataModel::Nullable<uint32_t> EnergyEvseDelegate::GetDischargingEnabledUntil()
{
    return mDischargingEnabledUntil;
}
CHIP_ERROR EnergyEvseDelegate::SetDischargingEnabledUntil(const DataModel::Nullable<uint32_t> &newValue)
{
    DataModel::Nullable<uint32_t> oldValue = mDischargingEnabledUntil;

    mDischargingEnabledUntil = newValue;
    if (oldValue != newValue) {
        if (newValue.IsNull()) {
            PRINTF_DEBUG("DischargingEnabledUntil updated to Null");
        } else {
            PRINTF_DEBUG("DischargingEnabledUntil updated to %lu",
                         static_cast<unsigned long int>(mDischargingEnabledUntil.Value()));
        }

        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, DischargingEnabledUntil::Id);
    }

    return CHIP_NO_ERROR;
}

int64_t EnergyEvseDelegate::GetCircuitCapacity()
{
    return mCircuitCapacity;
}

int64_t EnergyEvseDelegate::GetMinimumChargeCurrent()
{
    return mMinimumChargeCurrent;
}
CHIP_ERROR EnergyEvseDelegate::SetMinimumChargeCurrent(int64_t newValue)
{
    int64_t oldValue = mMinimumChargeCurrent;
    if (newValue < 0) {
        return CHIP_IM_GLOBAL_STATUS(ConstraintError);
    }

    mMinimumChargeCurrent = newValue;
    if (oldValue != mMinimumChargeCurrent) {
        PRINTF_DEBUG("MinimumChargeCurrent updated to %d mA", static_cast<int>(mMinimumChargeCurrent));
        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, MinimumChargeCurrent::Id);
    }
    return CHIP_NO_ERROR;
}

int64_t EnergyEvseDelegate::GetMaximumChargeCurrent()
{
    return mMaximumChargeCurrent;
}
CHIP_ERROR EnergyEvseDelegate::SetMaximumChargeCurrent(int64_t newValue)
{
    int64_t oldValue = mMaximumChargeCurrent;
    if (newValue < 0) {
        return CHIP_IM_GLOBAL_STATUS(ConstraintError);
    }

    mMaximumChargeCurrent = newValue;
    if (oldValue != mMaximumChargeCurrent) {
        PRINTF_DEBUG("Max Charging Current updated to %d mA", static_cast<int>(mMaximumChargeCurrent));
        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, MaximumChargeCurrent::Id);
    }
    return CHIP_NO_ERROR;
}

int64_t EnergyEvseDelegate::GetMaximumDischargeCurrent()
{
    return mMaximumDischargeCurrent;
}
CHIP_ERROR EnergyEvseDelegate::SetMaximumDischargeCurrent(int64_t newValue)
{
    int64_t oldValue = mMaximumDischargeCurrent;

    if (newValue < 0) {
        return CHIP_IM_GLOBAL_STATUS(ConstraintError);
    }

    mMaximumDischargeCurrent = newValue;
    if (oldValue != mMaximumDischargeCurrent) {
        PRINTF_DEBUG("MaximumDischargeCurrent updated to %ld", static_cast<long>(mMaximumDischargeCurrent));
        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, MaximumDischargeCurrent::Id);
    }
    return CHIP_NO_ERROR;
}

int64_t EnergyEvseDelegate::GetUserMaximumChargeCurrent()
{
    return mUserMaximumChargeCurrent;
}
CHIP_ERROR EnergyEvseDelegate::SetUserMaximumChargeCurrent(int64_t newValue)
{
    if (newValue < 0) {
        return CHIP_IM_GLOBAL_STATUS(ConstraintError);
    }

    int64_t oldValue = mUserMaximumChargeCurrent;
    mUserMaximumChargeCurrent = newValue;
    if (oldValue != newValue) {
        PRINTF_DEBUG("UserMaximumChargeCurrent updated to %ld", static_cast<long>(mUserMaximumChargeCurrent));

        ComputeMaxChargeCurrentLimit();

        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, UserMaximumChargeCurrent::Id);
    }

    return CHIP_NO_ERROR;
}

uint32_t EnergyEvseDelegate::GetRandomizationDelayWindow()
{
    return mRandomizationDelayWindow;
}
CHIP_ERROR EnergyEvseDelegate::SetRandomizationDelayWindow(uint32_t newValue)
{
    uint32_t oldValue = mRandomizationDelayWindow;
    if (newValue > kMaxRandomizationDelayWindow) {
        return CHIP_IM_GLOBAL_STATUS(ConstraintError);
    }

    mRandomizationDelayWindow = newValue;
    if (oldValue != newValue) {
        PRINTF_DEBUG("RandomizationDelayWindow updated to %lu",
                     static_cast<unsigned long int>(mRandomizationDelayWindow));

        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, RandomizationDelayWindow::Id);
    }
    return CHIP_NO_ERROR;
}

/* PREF attributes */
DataModel::Nullable<uint32_t> EnergyEvseDelegate::GetNextChargeStartTime()
{
    return mNextChargeStartTime;
}
CHIP_ERROR EnergyEvseDelegate::SetNextChargeStartTime(const DataModel::Nullable<uint32_t> &newNextChargeStartTimeUtc)
{
    if (newNextChargeStartTimeUtc == mNextChargeStartTime) {
        return CHIP_NO_ERROR;
    }

    mNextChargeStartTime = newNextChargeStartTimeUtc;
    if (mNextChargeStartTime.IsNull()) {
        PRINTF_DEBUG("NextChargeStartTime updated to Null");
    } else {
        std::string logs(20, '\0');
        GetReadableTime(mNextChargeStartTime.Value(), logs.data(), logs.size());
        PRINTF_DEBUG("NextChargeStartTime updated to %s", logs.c_str());
    }

    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, NextChargeStartTime::Id);

    return CHIP_NO_ERROR;
}

DataModel::Nullable<uint32_t> EnergyEvseDelegate::GetNextChargeTargetTime()
{
    return mNextChargeTargetTime;
}
CHIP_ERROR EnergyEvseDelegate::SetNextChargeTargetTime(const DataModel::Nullable<uint32_t> &newNextChargeTargetTimeUtc)
{
    if (newNextChargeTargetTimeUtc == mNextChargeTargetTime) {
        return CHIP_NO_ERROR;
    }

    mNextChargeTargetTime = newNextChargeTargetTimeUtc;
    if (mNextChargeTargetTime.IsNull()) {
        PRINTF_DEBUG("NextChargeTargetTime updated to Null");
    } else {
        std::string logs(20, '\0');
        GetReadableTime(mNextChargeTargetTime.Value(), logs.data(), sizeof(logs));
        PRINTF_DEBUG("NextChargeTargetTime updated to %s", logs.c_str());
    }

    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, NextChargeTargetTime::Id);

    return CHIP_NO_ERROR;
}

DataModel::Nullable<int64_t> EnergyEvseDelegate::GetNextChargeRequiredEnergy()
{
    return mNextChargeRequiredEnergy;
}
CHIP_ERROR EnergyEvseDelegate::SetNextChargeRequiredEnergy(
    const DataModel::Nullable<int64_t> &newNextChargeRequiredEnergyMilliWattH)
{
    if (mNextChargeRequiredEnergy == newNextChargeRequiredEnergyMilliWattH) {
        return CHIP_NO_ERROR;
    }

    mNextChargeRequiredEnergy = newNextChargeRequiredEnergyMilliWattH;
    if (mNextChargeRequiredEnergy.IsNull()) {
        PRINTF_DEBUG("NextChargeRequiredEnergy updated to Null");
    } else {
        PRINTF_DEBUG("NextChargeRequiredEnergy updated to %ld", static_cast<long>(mNextChargeRequiredEnergy.Value()));
    }

    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, NextChargeRequiredEnergy::Id);

    return CHIP_NO_ERROR;
}

DataModel::Nullable<Percent> EnergyEvseDelegate::GetNextChargeTargetSoC()
{
    return mNextChargeTargetSoC;
}
CHIP_ERROR EnergyEvseDelegate::SetNextChargeTargetSoC(const DataModel::Nullable<Percent> &newValue)
{
    DataModel::Nullable<Percent> oldValue = mNextChargeTargetSoC;

    mNextChargeTargetSoC = newValue;
    if (oldValue != newValue) {
        if (newValue.IsNull()) {
            PRINTF_DEBUG("NextChargeTargetSoC updated to Null");
        } else {
            PRINTF_DEBUG("NextChargeTargetSoC updated to %d %%", mNextChargeTargetSoC.Value());
        }
        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, NextChargeTargetSoC::Id);
    }
    return CHIP_NO_ERROR;
}

/* ApproximateEVEfficiency */
DataModel::Nullable<uint16_t> EnergyEvseDelegate::GetApproximateEVEfficiency()
{
    return mApproximateEVEfficiency;
}

CHIP_ERROR EnergyEvseDelegate::SetApproximateEVEfficiency(DataModel::Nullable<uint16_t> newValue)
{
    DataModel::Nullable<uint16_t> oldValue = mApproximateEVEfficiency;

    mApproximateEVEfficiency = newValue;
    if (oldValue != newValue) {
        if (newValue.IsNull()) {
            PRINTF_DEBUG("ApproximateEVEfficiency updated to Null");
        } else {
            PRINTF_DEBUG("ApproximateEVEfficiency updated to %d", mApproximateEVEfficiency.Value());
        }

        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, ApproximateEVEfficiency::Id);
    }

    return CHIP_NO_ERROR;
}

/* SOC attributes */
DataModel::Nullable<Percent> EnergyEvseDelegate::GetStateOfCharge()
{
    return mStateOfCharge;
}
DataModel::Nullable<int64_t> EnergyEvseDelegate::GetBatteryCapacity()
{
    return mBatteryCapacity;
}
/* PNC attributes*/
DataModel::Nullable<CharSpan> EnergyEvseDelegate::GetVehicleID()
{
    return mVehicleID;
}
/* Session SESS attributes */
DataModel::Nullable<uint32_t> EnergyEvseDelegate::GetSessionID()
{
    return mSession.mSessionID;
}
DataModel::Nullable<uint32_t> EnergyEvseDelegate::GetSessionDuration()
{
    return mSession.mSessionDuration;
}
DataModel::Nullable<int64_t> EnergyEvseDelegate::GetSessionEnergyCharged()
{
    return mSession.mSessionEnergyCharged;
}
DataModel::Nullable<int64_t> EnergyEvseDelegate::GetSessionEnergyDischarged()
{
    return mSession.mSessionEnergyDischarged;
}

void EvseSession::SetEndpointId(EndpointId aEndpoint)
{
    mEndpointId = aEndpoint;
}

EvseTargetsDelegate *EnergyEvseDelegate::GetEvseTargetsDelegate()
{
    return &mEvseTargetsDelegate;
}

void EvseSession::StartSession(int64_t currentEnergy)
{
    /* Get Timestamp */

    uint32_t chipEpoch = 0;
    CHIP_ERROR err = DeviceEnergyManagement::GetEpochTS(chipEpoch);

    if (err != CHIP_NO_ERROR) {
        /* Note that the error will be also be logged inside GetErrorTS() -
         * adding context here to help debugging */
        PRINTF_DEBUG("EVSE: Unable to get current time when starting session - err:%" CHIP_ERROR_FORMAT,
                     CHIP_ERR_FMT(err));
        return;
    }
    mStartTime = chipEpoch;

    mSessionEnergyChargedAtStart = currentEnergy;

    PRINTF_DEBUG("starting session at time %u, charging meter %lld", mStartTime,
                 static_cast<long long>(mSessionEnergyChargedAtStart));

    if (mSessionID.IsNull()) {
        mSessionID = MakeNullable(static_cast<uint32_t>(0));
    } else {
        uint32_t sessionID = mSessionID.Value() + 1;
        mSessionID = MakeNullable(sessionID);
    }

    /* Reset other session values */
    mSessionDuration = MakeNullable(static_cast<uint32_t>(0));
    mSessionEnergyCharged = MakeNullable(static_cast<int64_t>(0));
    mSessionEnergyDischarged = MakeNullable(static_cast<int64_t>(0));

    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, SessionID::Id);
    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, SessionDuration::Id);
    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, SessionEnergyCharged::Id);
    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, SessionEnergyDischarged::Id);
}

void EvseSession::StopSession(int64_t currentEnergy)
{
    RecalculateSessionDuration();
    UpdateEnergyCharged(currentEnergy);
}

/*---------------------- EvseSession functions --------------------------*/
void EvseSession::RecalculateSessionDuration()
{
    /* Get Timestamp */
    uint32_t chipEpoch = 0;
    CHIP_ERROR err = DeviceEnergyManagement::GetEpochTS(chipEpoch);

    if (err != CHIP_NO_ERROR) {
        /* Note that the error will be also be logged inside GetErrorTS() -
         * adding context here to help debugging */
        PRINTF_DEBUG("EVSE: Unable to get current time when updating session duration - err:%" CHIP_ERROR_FORMAT,
                     CHIP_ERR_FMT(err));
        return;
    }

    uint32_t duration = chipEpoch - mStartTime;
    mSessionDuration = MakeNullable(duration);
    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, SessionDuration::Id);

    PRINTF_DEBUG("Recalculate Session Duration: %u, mSessionEnergyCharged %lld", duration,
                 mSessionEnergyCharged.Value());
}

void EvseSession::UpdateEnergyCharged(int64_t currentEnergy)
{
    PRINTF_DEBUG("%lld - %lld", currentEnergy, mSessionEnergyChargedAtStart);
    mSessionEnergyCharged = MakeNullable(currentEnergy - mSessionEnergyChargedAtStart);
    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, SessionEnergyCharged::Id);
}

void EvseSession::UpdateEnergyDischarged(int64_t currentEnergy)
{
    mSessionEnergyDischarged = MakeNullable(currentEnergy - mSessionEnergyDischargedAtStart);
    MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, SessionEnergyDischarged::Id);
}

void EnergyEvseDelegate::ApplicationCallbackHandler(const EVSECbInfo *cb, intptr_t arg)
{
    auto pClass = reinterpret_cast<EnergyEvseDelegate *>(arg);

    switch (cb->type) {
    case EVSECallbackType::StateChanged: {
        PRINTF_DEBUG("EVSE callback - state changed");
        pClass->ComputeChargingSchedule();
        break;
    }
    case EVSECallbackType::ChargeCurrentChanged: {
        auto currentLimit = static_cast<int>(cb->ChargingCurrent.maximumChargeCurrent);
        PRINTF_DEBUG("EVSE callback - maxChargeCurrent changed to %d", currentLimit);

        pClass->ComputeChargingSchedule();
        ChargerManager::Controller().setChargingSessionCurrentLimit(currentLimit);

        break;
    }
    case EVSECallbackType::EnergyMeterReadingRequested: {
        PRINTF_DEBUG("EVSE callback - EnergyMeterReadingRequested");
        break;
    }

    case EVSECallbackType::ChargingPreferencesChanged: {
        PRINTF_DEBUG("EVSE callback - ChargingPreferencesChanged");
        pClass->ComputeChargingSchedule();
        break;
    }
    default: {
        PRINTF_DEBUG("Unhandled EVSE Callback type %d", static_cast<int>(cb->type));
    }
    }
}

Status EnergyEvseDelegate::HwRegisterEvseCallbackHandler(EVSECallbackFunc handler, intptr_t arg)
{
    if (mCallbacks.handler != nullptr) {
        PRINTF_DEBUG("Callback handler already initialized");
        return Status::Failure;
    }
    mCallbacks.handler = handler;
    mCallbacks.arg = arg;

    return Status::Success;
}

Status EnergyEvseDelegate::NotifyApplicationCurrentLimitChange(int64_t maximumChargeCurrent) const
{
    EVSECbInfo cbInfo;

    cbInfo.type = EVSECallbackType::ChargeCurrentChanged;
    cbInfo.ChargingCurrent.maximumChargeCurrent = maximumChargeCurrent;

    if (mCallbacks.handler != nullptr) {
        mCallbacks.handler(&cbInfo, mCallbacks.arg);
    }

    return Status::Success;
}

Status EnergyEvseDelegate::NotifyApplicationStateChange() const
{
    EVSECbInfo cbInfo;

    cbInfo.type = EVSECallbackType::StateChanged;
    cbInfo.StateChange.state = mState;
    cbInfo.StateChange.supplyState = mSupplyState;

    if (mCallbacks.handler != nullptr) {
        mCallbacks.handler(&cbInfo, mCallbacks.arg);
    }

    return Status::Success;
}

Status EnergyEvseDelegate::NotifyApplicationChargingPreferencesChange() const
{
    EVSECbInfo cbInfo;

    cbInfo.type = EVSECallbackType::ChargingPreferencesChanged;

    if (mCallbacks.handler != nullptr) {
        mCallbacks.handler(&cbInfo, mCallbacks.arg);
    }

    return Status::Success;
}

/**
 * @brief    Called by EVSE Hardware to notify the delegate of the maximum
 *           current limit supported by the hardware.
 *
 *           This is normally called at start-up.
 *
 * @param    currentmA - Maximum current limit supported by the hardware
 */
Status EnergyEvseDelegate::HwSetMaxHardwareCurrentLimit(int64_t currentmA)
{
    if (currentmA < kMinimumChargeCurrent) {
        return Status::ConstraintError;
    }

    /* there is no attribute to store this so store in private variable */
    mMaxHardwareCurrentLimit = currentmA;

    return ComputeMaxChargeCurrentLimit();
}

Status EnergyEvseDelegate::SendEvent_DetectedCard(ByteSpan uid)
{
    Events::Rfid::Type event{.uid = uid};

    chip::DeviceLayer::SystemLayer().ScheduleLambda([this, event]() {
        EventNumber eventNumber;
        CHIP_ERROR error = LogEvent(event, mEndpointId, eventNumber);
        if (CHIP_NO_ERROR != error) {
            PRINTF_DEBUG("[Notify] Unable to send notify event: %s [endpointId=%d]", error.AsString(), mEndpointId);
        }
    });
    return Status::Success;
}

Status EnergyEvseDelegate::SendFaultEvent(FaultStateEnum newFaultState)
{
    Events::Fault::Type event{
        .sessionID = mSession.mSessionID, // Note here the event sessionID can be Null!
        .state = mState, // This is the state prior to the fault being raised
        .faultStatePreviousState = mFaultState,
        .faultStateCurrentState = newFaultState,
    };

    chip::DeviceLayer::SystemLayer().ScheduleLambda([this, event]() {
        EventNumber eventNumber;
        CHIP_ERROR error = LogEvent(event, mEndpointId, eventNumber);
        if (CHIP_NO_ERROR != error) {
            PRINTF_DEBUG("Unable to send notify event: %" CHIP_ERROR_FORMAT, CHIP_ERR_FMT(error));
        }
    });

    return Status::Success;
}

Status EnergyEvseDelegate::HwSetFault(FaultStateEnum newFaultState)
{
    if (mFaultState == newFaultState) {
        PRINTF_DEBUG("No change in fault state, ignoring call");
        return Status::Failure;
    }

    /** Before we do anything we log the fault
     * any change in FaultState reports previous fault and new fault
     * and the state prior to the fault being raised */
    SendFaultEvent(newFaultState);

    /* Updated FaultState before we call into the handlers */
    SetFaultState(newFaultState);

    if (newFaultState == FaultStateEnum::kNoError) {
        /* Fault has been cleared */
        HandleFaultCleared();
    } else {
        /* a new Fault has been raised */
        HandleFaultRaised();
    }

    return Status::Success;
}

Status EnergyEvseDelegate::HandleFaultRaised()
{
    /* Save the current State and SupplyState so we can restore them if the fault clears */
    if (mStateBeforeFault == StateEnum::kUnknownEnumValue) {
        /* No existing fault - save this value to restore it later if it clears */
        mStateBeforeFault = mState;
    }

    if (mSupplyStateBeforeFault == SupplyStateEnum::kUnknownEnumValue) {
        /* No existing fault */
        mSupplyStateBeforeFault = mSupplyState;
    }

    /* Update State & SupplyState */
    SetState(StateEnum::kFault);
    SetSupplyState(SupplyStateEnum::kDisabledError);

    return Status::Success;
}

Status EnergyEvseDelegate::HandleFaultCleared()
{
    /* Check that something strange hasn't happened */
    if ((mStateBeforeFault == StateEnum::kUnknownEnumValue) ||
        (mSupplyStateBeforeFault == SupplyStateEnum::kUnknownEnumValue)) {
        PRINTF_DEBUG("EVSE: Something wrong trying to clear fault");
        return Status::Failure;
    }

    /* Restore the State and SupplyState back to old values once all the faults have cleared
     * Changing the State should notify the application, so it can continue charging etc
     */
    SetState(mStateBeforeFault);
    SetSupplyState(mSupplyStateBeforeFault);

    /* put back the sentinel to catch new faults if more are raised */
    mStateBeforeFault = StateEnum::kUnknownEnumValue;
    mSupplyStateBeforeFault = SupplyStateEnum::kUnknownEnumValue;

    return Status::Success;
}

Status EnergyEvseDelegate::SendEVConnectedEvent()
{
    Events::EVConnected::Type event;

    if (mSession.mSessionID.IsNull()) {
        PRINTF_DEBUG("SessionID is Null");
        return Status::Failure;
    }
    event.sessionID = mSession.mSessionID.Value();

    chip::DeviceLayer::SystemLayer().ScheduleLambda([this, event]() {
        EventNumber eventNumber;
        CHIP_ERROR error = LogEvent(event, mEndpointId, eventNumber);
        if (CHIP_NO_ERROR != error) {
            PRINTF_DEBUG("Unable to send notify event: %" CHIP_ERROR_FORMAT, CHIP_ERR_FMT(error));
        }
    });

    return Status::Success;
}

Status EnergyEvseDelegate::SendEVNotDetectedEvent()
{
    if (mSession.mSessionID.IsNull()) {
        PRINTF_DEBUG("SessionID is Null");
        return Status::Failure;
    }

    chip::DeviceLayer::SystemLayer().ScheduleLambda([this]() {
        Events::EVNotDetected::Type event;
        EventNumber eventNumber;

        event.sessionID = mSession.mSessionID.Value();
        event.state = mState;
        event.sessionDuration = mSession.mSessionDuration.Value();
        event.sessionEnergyCharged = mSession.mSessionEnergyCharged.Value();
        event.sessionEnergyDischarged = MakeOptional(mSession.mSessionEnergyDischarged.Value());

        CHIP_ERROR error = LogEvent(event, mEndpointId, eventNumber);

        if (CHIP_NO_ERROR != error) {
            PRINTF_DEBUG("Unable to send notify event: %" CHIP_ERROR_FORMAT, CHIP_ERR_FMT(error));
        }
    });

    return Status::Success;
}

Status EnergyEvseDelegate::SendEnergyTransferStartedEvent()
{
    if (mSession.mSessionID.IsNull()) {
        ChipLogError(AppServer, "SessionID is Null");
        return Status::Failure;
    }

    chip::DeviceLayer::SystemLayer().ScheduleLambda([this]() {
        Events::EnergyTransferStarted::Type event;
        EventNumber eventNumber;

        event.sessionID = mSession.mSessionID.Value();
        event.state = mState;
        /**
         * A positive value indicates the EV has been enabled for charging and the value is
         * taken directly from the MaximumChargeCurrent attribute.
         * A negative value indicates that the EV has been enabled for discharging and the value can be taken
         * from the MaximumDischargeCurrent attribute with its sign inverted.
         */
        if (mState == StateEnum::kPluggedInCharging) {
            /* Sample the energy meter for charging */
            event.maximumCurrent = mMaximumChargeCurrent;
        } else if (mState == StateEnum::kPluggedInDischarging) {
            /* Sample the energy meter for discharging */

            /* discharging should have a negative current  */
            event.maximumCurrent = -mMaximumDischargeCurrent;
        }

        CHIP_ERROR error = LogEvent(event, mEndpointId, eventNumber);
        if (CHIP_NO_ERROR != error) {
            PRINTF_DEBUG("Unable to send notify event: %" CHIP_ERROR_FORMAT, CHIP_ERR_FMT(error));
        }
    });

    return Status::Success;
}

Status EnergyEvseDelegate::SendEnergyTransferStoppedEvent(EnergyTransferStoppedReasonEnum reason)
{
    if (mSession.mSessionID.IsNull()) {
        ChipLogError(AppServer, "SessionID is Null");
        return Status::Failure;
    }

    chip::DeviceLayer::SystemLayer().ScheduleLambda([this, reason]() {
        Events::EnergyTransferStopped::Type event;
        EventNumber eventNumber;

        event.sessionID = mSession.mSessionID.Value();
        event.state = mState;
        event.reason = reason;

        if (mState == StateEnum::kPluggedInCharging) {
            event.energyTransferred = mSession.mSessionEnergyCharged.Value();
        } else if (mState == StateEnum::kPluggedInDischarging) {
            /* discharging should have a negative value */
            event.energyTransferred = mSession.mSessionEnergyDischarged.Value();
        }

        CHIP_ERROR error = LogEvent(event, mEndpointId, eventNumber);
        if (CHIP_NO_ERROR != error) {
            PRINTF_DEBUG("Unable to send notify event: %" CHIP_ERROR_FORMAT, CHIP_ERR_FMT(error));
        }
    });

    return Status::Success;
}

/**
 *  @brief   Called to compute the safe charging current limit
 *
 * mActualChargingCurrentLimit is the minimum of:
 *   - MaxHardwareCurrentLimit (of the hardware)
 *   - CircuitCapacity (set by the electrician - less than the hardware)
 *   - CableAssemblyLimit (detected when the cable is inserted)
 *   - MaximumChargeCurrent (from charging command)
 *   - UserMaximumChargeCurrent (could dynamically change)
 *
 */
Status EnergyEvseDelegate::ComputeMaxChargeCurrentLimit()
{
    int64_t oldValue;

    oldValue = mActualChargingCurrentLimit;
    mActualChargingCurrentLimit = mMaxHardwareCurrentLimit;
    mActualChargingCurrentLimit = min(mActualChargingCurrentLimit, mMaximumChargingCurrentLimitFromCommand);
    mActualChargingCurrentLimit = min(mActualChargingCurrentLimit, mUserMaximumChargeCurrent);

    /* Set the actual max charging current attribute */
    mMaximumChargeCurrent = mActualChargingCurrentLimit;

    if (oldValue != mMaximumChargeCurrent) {
        PRINTF_DEBUG("MaximumChargeCurrent updated to %ld", static_cast<long>(mMaximumChargeCurrent));
        MatterManager::ReportAttributeChangeToMatter(mEndpointId, EnergyEvse::Id, MaximumChargeCurrent::Id);

        /* Call the EV Charger hardware current limit callback */
        NotifyApplicationCurrentLimitChange(mMaximumChargeCurrent);
    }
    return Status::Success;
}

CHIP_ERROR EnergyEvseDelegate::FindNextTarget(const BitMask<EnergyEvse::TargetDayOfWeekBitmap> dayOfWeekMap,
                                              uint16_t minutesPastMidnightNow_m,
                                              uint16_t &targetTimeMinutesPastMidnight_m,
                                              DataModel::Nullable<Percent> &targetSoC,
                                              DataModel::Nullable<int64_t> &addedEnergy_mWh, bool bAllowTargetsInPast)
{
    uint16_t minTimeToTarget_m = 24 * 60; // 24 hours
    bool bFound = false;

    const DataModel::List<const EnergyEvse::Structs::ChargingTargetScheduleStruct::Type> &chargingTargetSchedules =
        mEvseTargetsDelegate.GetTargets();
    for (auto &chargingTargetScheduleEntry : chargingTargetSchedules) {
        if (!chargingTargetScheduleEntry.dayOfWeekForSequence.HasAny(dayOfWeekMap)) {
            continue;
        }

        for (auto &chargingTarget : chargingTargetScheduleEntry.chargingTargets) {
            if (TargetSkippedAsPast(chargingTarget.targetTimeMinutesPastMidnight, minutesPastMidnightNow_m,
                                    bAllowTargetsInPast)) {
                continue;
            }

            TakeEarlierTarget(chargingTarget, minTimeToTarget_m, bFound, targetTimeMinutesPastMidnight_m, targetSoC,
                              addedEnergy_mWh);
        }

        if (bFound) {
            break;
        }
    }

    return bFound ? CHIP_NO_ERROR : CHIP_ERROR_NOT_FOUND;
}

void EnergyEvseDelegate::AdvanceScheduleSearchDay(BitMask<EnergyEvse::TargetDayOfWeekBitmap> &dayOfWeekMap)
{
    const std::byte nextDayMask =
        (std::byte{static_cast<uint8_t>(dayOfWeekMap.Raw())} << 1) & std::byte{kAllTargetDaysMask};
    dayOfWeekMap = BitMask<EnergyEvse::TargetDayOfWeekBitmap>(std::to_integer<uint8_t>(nextDayMask));

    if (!dayOfWeekMap.HasAny()) {
        dayOfWeekMap = BitMask<EnergyEvse::TargetDayOfWeekBitmap>(TargetDayOfWeekBitmap::kSunday);
    }
}

CHIP_ERROR EnergyEvseDelegate::SearchNextChargeTargetAcrossDays(
    BitMask<EnergyEvse::TargetDayOfWeekBitmap> &dayOfWeekMap, uint16_t minutesPastMidnightNow_m,
    uint16_t &targetTimeMinutesPastMidnight_m, DataModel::Nullable<Percent> &targetSoC,
    DataModel::Nullable<int64_t> &addedEnergy_mWh, uint8_t &searchDay)
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    searchDay = 0;
    bool done = false;

    while (searchDay < 2 && !done) {
        PRINTF_DEBUG("Searching for target on %s (searchDay=%u)", GetDayOfWeekStr(dayOfWeekMap), searchDay);

        err = FindNextTarget(dayOfWeekMap, minutesPastMidnightNow_m, targetTimeMinutesPastMidnight_m, targetSoC,
                             addedEnergy_mWh, (searchDay != 0));

        if (err == CHIP_ERROR_NOT_FOUND) {
            PRINTF_DEBUG("No more targets found for %s", GetDayOfWeekStr(dayOfWeekMap));
            searchDay++;
            AdvanceScheduleSearchDay(dayOfWeekMap);
            continue;
        }

        if (err == CHIP_NO_ERROR) {
            PRINTF_DEBUG("Found target for %s at %u minutes past midnight", GetDayOfWeekStr(dayOfWeekMap),
                         targetTimeMinutesPastMidnight_m);
            done = true;
            continue;
        }

        PRINTF_DEBUG("Error during FindNextTarget: %" CHIP_ERROR_FORMAT, CHIP_ERR_FMT(err));
        done = true;
    }

    return err;
}

CHIP_ERROR EnergyEvseDelegate::FillNextChargeScheduleTimes(uint8_t searchDay, uint16_t targetTimeMinutesPastMidnight_m,
                                                           uint16_t minutesPastMidnightNow_m, uint32_t now_epoch_s,
                                                           DataModel::Nullable<Percent> &targetSoC,
                                                           DataModel::Nullable<int64_t> &addedEnergy_mWh,
                                                           DataModel::Nullable<uint32_t> &startTime_epoch_s,
                                                           DataModel::Nullable<uint32_t> &targetTime_epoch_s)
{
    uint32_t tempTargetTime_epoch_s =
        ((now_epoch_s / 60) + targetTimeMinutesPastMidnight_m + (searchDay * 1440) - minutesPastMidnightNow_m) * 60;
    targetTime_epoch_s.SetNonNull(tempTargetTime_epoch_s);

    if (!targetSoC.IsNull()) {
        std::string targetTimeBuf(20, '\0');
        GetReadableTime(tempTargetTime_epoch_s, targetTimeBuf.data(), targetTimeBuf.size());

        PRINTF_DEBUG("Schedule using SoC: Target=%u%%, TargetTime=[%s]", targetSoC.Value(), targetTimeBuf.c_str());

        if (targetSoC.Value() != 100) {
            PRINTF_DEBUG("EVSE WARNING: TargetSoC is not 100%% and we don't know the EV SoC!");
        }
        startTime_epoch_s.SetNonNull(now_epoch_s);
        return CHIP_NO_ERROR;
    }

    if (addedEnergy_mWh.IsNull()) {
        PRINTF_DEBUG("EVSE ERROR: Neither TargetSoC or AddedEnergy has been provided");
        return CHIP_ERROR_INTERNAL;
    }

    uint32_t power_W = static_cast<uint32_t>((230 * GetMaximumChargeCurrent()) / 1000);
    if (power_W == 0) {
        PRINTF_DEBUG("EVSE Error: MaxCurrent = 0Amp - Can't schedule charging");
        return CHIP_ERROR_INTERNAL;
    }

    uint32_t chargingDuration_s = static_cast<uint32_t>(((addedEnergy_mWh.Value() / power_W) * 36) / 10);
    chargingDuration_s += (15 * 60);
    uint32_t tempStartTime_epoch_s = tempTargetTime_epoch_s - chargingDuration_s;

    PRINTF_DEBUG("Schedule using Energy: Required=%lldmWh, Power=%uW, Duration=%us", addedEnergy_mWh.Value(), power_W,
                 chargingDuration_s);

    if (tempStartTime_epoch_s < now_epoch_s) {
        startTime_epoch_s.SetNonNull(now_epoch_s);
        PRINTF_DEBUG("Enable EV to start charging");
    } else {
        startTime_epoch_s.SetNonNull(tempStartTime_epoch_s);
        PRINTF_DEBUG("Disable EV to start charging");
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR EnergyEvseDelegate::ComputeChargingSchedule()
{
    if (!MatterManager::GetInstance().isConnected) {
        PRINTF_DEBUG("MatterManager::GetInstance() is NOT Connected");
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR err = CHIP_NO_ERROR;

    BitMask<EnergyEvse::TargetDayOfWeekBitmap> dayOfWeekMap;
    ReturnErrorOnFailure(chip::app::Clusters::DeviceEnergyManagement::GetLocalDayOfWeekNow(dayOfWeekMap));

    uint16_t minutesPastMidnightNow_m = 0;
    ReturnErrorOnFailure(chip::app::Clusters::DeviceEnergyManagement::GetMinutesPastMidnight(minutesPastMidnightNow_m));

    uint32_t now_epoch_s = 0;
    ReturnErrorOnFailure(chip::app::Clusters::DeviceEnergyManagement::GetEpochTS(now_epoch_s));

    // LOG: Entry state with human-readable conversions
    std::string humanTimeBuf(20, '\0');
    GetReadableTime(now_epoch_s, humanTimeBuf.data(), humanTimeBuf.size());

    PRINTF_DEBUG("--- ComputeChargingSchedule Start ---");
    PRINTF_DEBUG("Current System Time: [%s]", humanTimeBuf.c_str());
    PRINTF_DEBUG("Current Day: %s (0x%02x)", GetDayOfWeekStr(dayOfWeekMap), dayOfWeekMap.Raw());
    PRINTF_DEBUG("Minutes Past Midnight: %u", minutesPastMidnightNow_m);

    DataModel::Nullable<uint32_t> startTime_epoch_s;
    DataModel::Nullable<uint32_t> targetTime_epoch_s;
    DataModel::Nullable<Percent> targetSoC;
    DataModel::Nullable<int64_t> addedEnergy_mWh;

    uint16_t targetTimeMinutesPastMidnight_m = 0;
    uint8_t searchDay = 0;

    targetTime_epoch_s.SetNull();
    targetSoC.SetNull();
    addedEnergy_mWh.SetNull();
    startTime_epoch_s.SetNull();

    if (!IsEvsePluggedIn() || GetSupplyState() != SupplyStateEnum::kChargingEnabled) {
        PRINTF_DEBUG("ComputeChargingSchedule: Not plugged in or charging disabled");
    } else {
        err = SearchNextChargeTargetAcrossDays(dayOfWeekMap, minutesPastMidnightNow_m, targetTimeMinutesPastMidnight_m,
                                               targetSoC, addedEnergy_mWh, searchDay);

        if (err == CHIP_NO_ERROR) {
            CHIP_ERROR const fillErr = FillNextChargeScheduleTimes(
                searchDay, targetTimeMinutesPastMidnight_m, minutesPastMidnightNow_m, now_epoch_s, targetSoC,
                addedEnergy_mWh, startTime_epoch_s, targetTime_epoch_s);
            if (fillErr != CHIP_NO_ERROR) {
                return fillErr;
            }
        } else {
            PRINTF_DEBUG("ComputeChargingSchedule: No target found or error occurred (err=%s)", ErrorStr(err));
        }
    }

    SetNextChargeStartTime(startTime_epoch_s);
    SetNextChargeTargetTime(targetTime_epoch_s);
    SetNextChargeRequiredEnergy(addedEnergy_mWh);
    SetNextChargeTargetSoC(targetSoC);

    // LOG: Final Summary
    std::string startBuf(20, '\0');
    std::string targetBuf(20, '\0');

    GetReadableTime(startTime_epoch_s.ValueOr(0), startBuf.data(), startBuf.size());
    GetReadableTime(targetTime_epoch_s.ValueOr(0), targetBuf.data(), targetBuf.size());

    PRINTF_DEBUG("Charge Profile Summary: Start=[%s], Target=[%s], SoC=%u%%, Energy=%lldmWh", startBuf.c_str(),
                 targetBuf.c_str(), targetSoC.ValueOr(0), addedEnergy_mWh.ValueOr(0));

    return err;
}

Status EnergyEvseDelegate::CheckFaultOrDiagnostic() const
{
    if (mFaultState != FaultStateEnum::kNoError) {
        PRINTF_DEBUG("EVSE: Trying to handle command when fault is present");
        return Status::Failure;
    }

    if (mSupplyState == SupplyStateEnum::kDisabledDiagnostics) {
        PRINTF_DEBUG("EVSE: Trying to handle command when in diagnostics mode");
        return Status::Failure;
    }
    return Status::Success;
}

Status EnergyEvseDelegate::HandleChargingEnabledEvent()
{
    /* Check there is no Fault or Diagnostics condition */
    Status status = CheckFaultOrDiagnostic();
    if (status != Status::Success) {
        return status;
    }

    /* update SupplyState to say that charging is now enabled */
    if constexpr (kSupportDischargingV2x) {
        SetSupplyState(SupplyStateEnum::kDischargingEnabled);
    } else {
        SetSupplyState(SupplyStateEnum::kChargingEnabled);
    }

    switch (mState) {
    case StateEnum::kNotPluggedIn:
    case StateEnum::kPluggedInNoDemand:
        break;
    case StateEnum::kPluggedInDemand:
        ComputeMaxChargeCurrentLimit();
        SetState(StateEnum::kPluggedInCharging);
        SendEnergyTransferStartedEvent();
        break;
    case StateEnum::kPluggedInCharging:
        break;
    case StateEnum::kPluggedInDischarging:
        /* Switched from discharging to charging */
        SendEnergyTransferStoppedEvent(EnergyTransferStoppedReasonEnum::kEVSEStopped);

        ComputeMaxChargeCurrentLimit();
        SetState(StateEnum::kPluggedInCharging);
        SendEnergyTransferStartedEvent();
        break;
    default:
        break;
    }

    ScheduleCheckOnEnabledTimeout();

    return Status::Success;
}

bool EnergyEvseDelegate::IsEvsePluggedIn() const
{
    return (mState == StateEnum::kPluggedInCharging || mState == StateEnum::kPluggedInDemand ||
            mState == StateEnum::kPluggedInDischarging || mState == StateEnum::kPluggedInNoDemand);
}

Status EnergyEvseDelegate::HandleDisabledEvent()
{
    /* Check there is no Fault or Diagnostics condition */
    Status status = CheckFaultOrDiagnostic();
    if (status != Status::Success) {
        return status;
    }

    /* update SupplyState to say that charging is now enabled */
    SetSupplyState(SupplyStateEnum::kDisabled);
    SendEnergyTransferStoppedEvent(EnergyTransferStoppedReasonEnum::kEVSEStopped);

    switch (mState) {
    case StateEnum::kNotPluggedIn:
    case StateEnum::kPluggedInNoDemand:
    case StateEnum::kPluggedInDemand:
        break;
    case StateEnum::kPluggedInCharging:
    case StateEnum::kPluggedInDischarging:
        SendEnergyTransferStoppedEvent(EnergyTransferStoppedReasonEnum::kEVSEStopped);
        break;
    default:
        break;
    }

    return Status::Success;
}

Status EnergyEvseDelegate::ScheduleCheckOnEnabledTimeout()
{
    uint32_t chipEpoch = 0;
    DataModel::Nullable<uint32_t> enabledUntilTime;

    if (mSupplyState == SupplyStateEnum::kChargingEnabled) {
        enabledUntilTime = GetChargingEnabledUntil();
    } else if (mSupplyState == SupplyStateEnum::kDischargingEnabled) {
        enabledUntilTime = GetDischargingEnabledUntil();
    } else {
        // In all other states the EVSE is disabled
        return Status::Success;
    }

    if (enabledUntilTime.IsNull()) {
        /* This is enabled indefinitely so don't schedule a callback */
        return Status::Success;
    }

    CHIP_ERROR err = DeviceEnergyManagement::GetEpochTS(chipEpoch);
    if (err == CHIP_NO_ERROR) {
        /* time is sync'd */
        auto delta = static_cast<int32_t>(enabledUntilTime.Value() - chipEpoch);
        if (delta > 0) {
            /* The timer hasn't expired yet - set a timer to check in the future */
            PRINTF_DEBUG("Setting EVSE Enable check timer for %ld seconds", static_cast<long int>(delta));
            DeviceLayer::SystemLayer().StartTimer(System::Clock::Seconds32(delta), EvseCheckTimerExpiry, this);
        } else {
            /* we have gone past the enabledUntilTime - so we need to disable */
            PRINTF_DEBUG("EVSE enable time expired, disabling charging");
            Disable();
        }
    } else if (err == CHIP_ERROR_REAL_TIME_NOT_SYNCED) {
        /* Real time isn't sync'd -lets check again in 30 seconds - otherwise keep the charger enabled */
        DeviceLayer::SystemLayer().StartTimer(
            System::Clock::Seconds32(kPeriodicCheckIntervalRealTimeClockNotSynced_sec), EvseCheckTimerExpiry, this);
    }

    return Status::Success;
}

void EnergyEvseDelegate::OnEvseEnableTimerExpired()
{
    ScheduleCheckOnEnabledTimeout();
}

void EnergyEvseDelegate::EvseCheckTimerExpiry([[maybe_unused]] System::Layer *systemLayer, void *callbackContext)
{
    // TimerCompleteCallback passes the context registered with StartTimer (we pass `this`).
    if (callbackContext == nullptr) {
        ChipLogError(AppServer, "EVSE timer callback context is null");
        return;
    }

    auto *delegate = static_cast<EnergyEvseDelegate *>(callbackContext);
    delegate->OnEvseEnableTimerExpired();
}

#ifdef UNIT_TEST
bool EnergyEvse_TargetSkippedAsPast(uint16_t targetMinutesPastMidnight, uint16_t minutesPastMidnightNow_m,
                                    bool allowTargetsInPast)
{
    return TargetSkippedAsPast(targetMinutesPastMidnight, minutesPastMidnightNow_m, allowTargetsInPast);
}
#endif