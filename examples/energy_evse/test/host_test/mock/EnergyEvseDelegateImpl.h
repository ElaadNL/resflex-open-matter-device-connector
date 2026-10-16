// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app/server/Server.h"
#include "chip_system_layer.h"
#include "esp_matter_evse.h"
#include <app/clusters/energy-evse-server/energy-evse-server.h>
#include <EVSECallbacks.h>

#include <EnergyEvseTargetsStore.h>

using chip::Protocols::InteractionModel::Status;

namespace chip {
namespace app {
namespace Clusters {
namespace EnergyEvse {

class EvseSession {
public:
    EvseSession() = default;

    void SetEndpointId(EndpointId aEndpoint);

    void StartSession(int64_t currentEnergy);
    void StopSession(int64_t currentEnergy);
    void RecalculateSessionDuration();
    void UpdateEnergyCharged(int64_t currentEnergy);
    void UpdateEnergyDischarged(int64_t currentEnergy);

    DataModel::Nullable<uint32_t> mSessionID;
    DataModel::Nullable<uint32_t> mSessionDuration;
    DataModel::Nullable<int64_t> mSessionEnergyCharged;
    DataModel::Nullable<int64_t> mSessionEnergyDischarged;

private:
    EndpointId mEndpointId = 0;
    uint32_t mStartTime = 0;
    int64_t mSessionEnergyChargedAtStart = 0;
    int64_t mSessionEnergyDischargedAtStart = 0;
};

class EnergyEvseDelegate : public Clusters::EnergyEvse::Delegate {
public:
    EnergyEvseDelegate();

    void SetupDelegate(EndpointId id);

    void AddCustomAttributes();
    void AddCustomFeatures(BitMask<Feature, uint32_t> aFeature);

    void LateSetupAfterMatter();

    Status Disable() override;
    Status EnableCharging(const DataModel::Nullable<uint32_t> &chargingEnabledUntil,
                          const int64_t &minimumChargeCurrent, const int64_t &maximumChargeCurrent) override;
    Status EnableDischarging(const DataModel::Nullable<uint32_t> &enableDischargeTime,
                             const int64_t &maximumDischargeCurrent) override;
    Status StartDiagnostics() override;
    Status SetTargets(const DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType>
                          &chargingTargetSchedules) override;
    Status LoadTargets() override;
    Status GetTargets(
        DataModel::List<const Structs::ChargingTargetScheduleStruct::Type> &chargingTargetSchedules) override;
    Status ClearTargets() override;

    StateEnum GetState() override;
    CHIP_ERROR SetState(StateEnum newValue);

    SupplyStateEnum GetSupplyState() override;
    CHIP_ERROR SetSupplyState(SupplyStateEnum newValue);

    FaultStateEnum GetFaultState() override;
    CHIP_ERROR SetFaultState(FaultStateEnum newValue);

    DataModel::Nullable<uint32_t> GetChargingEnabledUntil() override;
    CHIP_ERROR SetChargingEnabledUntil(const DataModel::Nullable<uint32_t> &newValue);

    DataModel::Nullable<uint32_t> GetDischargingEnabledUntil() override;
    CHIP_ERROR SetDischargingEnabledUntil(const DataModel::Nullable<uint32_t> &newValue);

    int64_t GetCircuitCapacity() override;
    int64_t GetMinimumChargeCurrent() override;
    CHIP_ERROR SetMinimumChargeCurrent(int64_t newValue);

    int64_t GetMaximumChargeCurrent() override;
    CHIP_ERROR SetMaximumChargeCurrent(int64_t newValue);

    int64_t GetMaximumDischargeCurrent() override;
    CHIP_ERROR SetMaximumDischargeCurrent(int64_t newValue);

    int64_t GetUserMaximumChargeCurrent() override;
    CHIP_ERROR SetUserMaximumChargeCurrent(int64_t newValue) override;

    uint32_t GetRandomizationDelayWindow() override;
    CHIP_ERROR SetRandomizationDelayWindow(uint32_t newValue) override;

    DataModel::Nullable<uint32_t> GetNextChargeStartTime() override;
    CHIP_ERROR SetNextChargeStartTime(const DataModel::Nullable<uint32_t> &newNextChargeStartTimeUtc);

    DataModel::Nullable<uint32_t> GetNextChargeTargetTime() override;
    CHIP_ERROR SetNextChargeTargetTime(const DataModel::Nullable<uint32_t> &newNextChargeTargetTimeUtc);

    DataModel::Nullable<int64_t> GetNextChargeRequiredEnergy() override;
    CHIP_ERROR SetNextChargeRequiredEnergy(const DataModel::Nullable<int64_t> &newNextChargeRequiredEnergyMilliWattH);

    DataModel::Nullable<Percent> GetNextChargeTargetSoC() override;
    CHIP_ERROR SetNextChargeTargetSoC(const DataModel::Nullable<Percent> &newValue);

    DataModel::Nullable<uint16_t> GetApproximateEVEfficiency() override;
    CHIP_ERROR SetApproximateEVEfficiency(DataModel::Nullable<uint16_t> newValue) override;

    DataModel::Nullable<Percent> GetStateOfCharge() override;
    DataModel::Nullable<int64_t> GetBatteryCapacity() override;
    DataModel::Nullable<CharSpan> GetVehicleID() override;
    DataModel::Nullable<uint32_t> GetSessionID() override;
    DataModel::Nullable<uint32_t> GetSessionDuration() override;
    DataModel::Nullable<int64_t> GetSessionEnergyCharged() override;
    DataModel::Nullable<int64_t> GetSessionEnergyDischarged() override;

    static constexpr int kDefaultMinChargeCurrent_mA = 6000;
    static constexpr int kDefaultUserMaximumChargeCurrent_mA = 32 * 1000;
    static constexpr int kDefaultRandomizationDelayWindow_sec = 600;
    static constexpr int kMaxVehicleIDBufSize = 32;
    static constexpr int kPeriodicCheckIntervalRealTimeClockNotSynced_sec = 30;

    StateEnum mState = StateEnum::kNotPluggedIn;
    SupplyStateEnum mSupplyState = SupplyStateEnum::kDisabled;
    FaultStateEnum mFaultState = FaultStateEnum::kNoError;
    DataModel::Nullable<uint32_t> mChargingEnabledUntil;
    DataModel::Nullable<uint32_t> mDischargingEnabledUntil;
    int64_t mCircuitCapacity = kDefaultUserMaximumChargeCurrent_mA;
    int64_t mMinimumChargeCurrent = kDefaultMinChargeCurrent_mA;
    int64_t mMaximumChargeCurrent = kDefaultUserMaximumChargeCurrent_mA;
    int64_t mMaximumDischargeCurrent = 0;
    int64_t mUserMaximumChargeCurrent = kDefaultUserMaximumChargeCurrent_mA;
    uint32_t mRandomizationDelayWindow = kDefaultRandomizationDelayWindow_sec;
    DataModel::Nullable<uint32_t> mNextChargeStartTime;
    DataModel::Nullable<uint32_t> mNextChargeTargetTime;
    DataModel::Nullable<int64_t> mNextChargeRequiredEnergy;
    DataModel::Nullable<Percent> mNextChargeTargetSoC;
    DataModel::Nullable<uint16_t> mApproximateEVEfficiency;
    DataModel::Nullable<Percent> mStateOfCharge;
    DataModel::Nullable<int64_t> mBatteryCapacity;
    DataModel::Nullable<CharSpan> mVehicleID;

    EvseSession mSession = EvseSession();

    Status ScheduleCheckOnEnabledTimeout();
    bool IsEvsePluggedIn() const;

    Status CheckFaultOrDiagnostic() const;
    Status HandleChargingEnabledEvent();
    Status HandleDisabledEvent();
    Status HandleFaultRaised();
    Status HandleFaultCleared();

    Status SendEVConnectedEvent();
    Status SendEVNotDetectedEvent();
    Status SendEnergyTransferStartedEvent();
    Status SendEnergyTransferStoppedEvent(EnergyTransferStoppedReasonEnum reason);

    Status ComputeMaxChargeCurrentLimit();
    CHIP_ERROR FindNextTarget(const BitMask<EnergyEvse::TargetDayOfWeekBitmap> dayOfWeekMap,
                              uint16_t minutesPastMidnightNow_m, uint16_t &targetTimeMinutesPastMidnight_m,
                              DataModel::Nullable<Percent> &targetSoC, DataModel::Nullable<int64_t> &addedEnergy_mWh,
                              bool bAllowTargetsInPast);
    CHIP_ERROR ComputeChargingSchedule();

    static void EvseCheckTimerExpiry(System::Layer *systemLayer, void *callbackContext);

    static void ApplicationCallbackHandler(const EVSECbInfo *cb, intptr_t arg);

    EVSECallbackWrapper mCallbacks = {.handler = nullptr, .arg = 0};
    Status HwRegisterEvseCallbackHandler(EVSECallbackFunc handler, intptr_t arg);
    Status NotifyApplicationCurrentLimitChange(int64_t maximumChargeCurrent) const;
    Status NotifyApplicationStateChange() const;
    Status NotifyApplicationChargingPreferencesChange() const;

    Status HwSetMaxHardwareCurrentLimit(int64_t currentmA);
    Status HwSetFault(FaultStateEnum fault);

    Status SendEvent_DetectedCard(ByteSpan uid);

    Status SendFaultEvent(FaultStateEnum newFaultState);

    EvseTargetsDelegate *GetEvseTargetsDelegate();

private:
    void AdvanceScheduleSearchDay(BitMask<EnergyEvse::TargetDayOfWeekBitmap> &dayOfWeekMap);

    CHIP_ERROR SearchNextChargeTargetAcrossDays(BitMask<EnergyEvse::TargetDayOfWeekBitmap> &dayOfWeekMap,
                                                uint16_t minutesPastMidnightNow_m,
                                                uint16_t &targetTimeMinutesPastMidnight_m,
                                                DataModel::Nullable<Percent> &targetSoC,
                                                DataModel::Nullable<int64_t> &addedEnergy_mWh, uint8_t &searchDay);

    CHIP_ERROR FillNextChargeScheduleTimes(uint8_t searchDay, uint16_t targetTimeMinutesPastMidnight_m,
                                           uint16_t minutesPastMidnightNow_m, uint32_t now_epoch_s,
                                           DataModel::Nullable<Percent> &targetSoC,
                                           DataModel::Nullable<int64_t> &addedEnergy_mWh,
                                           DataModel::Nullable<uint32_t> &startTime_epoch_s,
                                           DataModel::Nullable<uint32_t> &targetTime_epoch_s);

    void OnEvseEnableTimerExpired();

    int64_t mMaxHardwareCurrentLimit = 0;
    int64_t mCableAssemblyCurrentLimit = 0;
    int64_t mMaximumChargingCurrentLimitFromCommand = 0;
    int64_t mActualChargingCurrentLimit = 0;

    StateEnum mStateBeforeFault = StateEnum::kUnknownEnumValue;
    SupplyStateEnum mSupplyStateBeforeFault = SupplyStateEnum::kUnknownEnumValue;

    BitMask<Feature> mFeature;

    EvseTargetsDelegate mEvseTargetsDelegate;
};

} // namespace EnergyEvse
} // namespace Clusters
} // namespace app
} // namespace chip
