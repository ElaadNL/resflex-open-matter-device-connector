// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <app/clusters/energy-evse-server/energy-evse-server.h>
#include <EVSECallbacks.h>

#include <EnergyEvseTargetsStore.h>

using chip::Protocols::InteractionModel::Status;

namespace chip {
namespace app {
namespace Clusters {
namespace EnergyEvse {

// A bitmap of all possible days (the union of the values in
// chip::app::Clusters::EnergyEvse::TargetDayOfWeekBitmap)
constexpr uint8_t kAllTargetDaysMask = 0x7f;

/**
 * Helper class to handle all of the session related info
 */
class EvseSession {
public:
    EvseSession() = default;

    void SetEndpointId(EndpointId aEndpoint);

    void StartSession(int64_t currentEnergy);
    void StopSession(int64_t currentEnergy);
    void RecalculateSessionDuration();
    void UpdateEnergyCharged(int64_t currentEnergy);
    void UpdateEnergyDischarged(int64_t currentEnergy);

    /* Public members - represent attributes in the cluster */
    DataModel::Nullable<uint32_t> mSessionID;
    DataModel::Nullable<uint32_t> mSessionDuration;
    DataModel::Nullable<int64_t> mSessionEnergyCharged;
    DataModel::Nullable<int64_t> mSessionEnergyDischarged;

private:
    EndpointId mEndpointId = 0; // saved the endpoint of the owner
    uint32_t mStartTime = 0; // Epoch_s - 0 means it hasn't started yet
    int64_t mSessionEnergyChargedAtStart = 0; // in mWh - 0 means it hasn't been set yet
    int64_t mSessionEnergyDischargedAtStart = 0; // in mWh - 0 means it hasn't been set yet
};

class EnergyEvseDelegate : public Clusters::EnergyEvse::Delegate {
public:
    EnergyEvseDelegate();

    void SetupDelegate(EndpointId id);

    // add custom attributes to the cluster, not to override the exisiting source codes
    void AddCustomAttributes();
    void AddCustomFeatures(Feature aFeature);

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
    /* PREF attributes */
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

    /* SOC attributes */
    DataModel::Nullable<Percent> GetStateOfCharge() override;
    DataModel::Nullable<int64_t> GetBatteryCapacity() override;
    /* PNC attributes*/
    DataModel::Nullable<CharSpan> GetVehicleID() override;
    /* Session SESS attributes */
    DataModel::Nullable<uint32_t> GetSessionID() override;
    DataModel::Nullable<uint32_t> GetSessionDuration() override;
    DataModel::Nullable<int64_t> GetSessionEnergyCharged() override;
    DataModel::Nullable<int64_t> GetSessionEnergyDischarged() override;

    static constexpr int kDefaultMinChargeCurrent_mA = 6000; /* 6A */
    /*
        10A for testing cable only, currently input cable limited to 13A
        actual input cable is based on installer
    */
    static constexpr int kDefaultUserMaximumChargeCurrent_mA = 32 * 1000; /*32 A*/
    static constexpr int kDefaultRandomizationDelayWindow_sec = 600; /* 600s */
    static constexpr int kMaxVehicleIDBufSize = 32;
    static constexpr int kPeriodicCheckIntervalRealTimeClockNotSynced_sec = 30;

    /* Attributes */
    StateEnum mState = StateEnum::kNotPluggedIn;
    SupplyStateEnum mSupplyState = SupplyStateEnum::kDisabled;
    FaultStateEnum mFaultState = FaultStateEnum::kNoError;
    DataModel::Nullable<uint32_t> mChargingEnabledUntil; // TODO Default to 0 to indicate disabled
    DataModel::Nullable<uint32_t> mDischargingEnabledUntil; // TODO Default to 0 to indicate disabled
    int64_t mCircuitCapacity = kDefaultUserMaximumChargeCurrent_mA;
    int64_t mMinimumChargeCurrent = kDefaultMinChargeCurrent_mA;
    int64_t mMaximumChargeCurrent = kDefaultUserMaximumChargeCurrent_mA;
    int64_t mMaximumDischargeCurrent = 0;
    int64_t mUserMaximumChargeCurrent = kDefaultUserMaximumChargeCurrent_mA; // TODO update spec
    uint32_t mRandomizationDelayWindow = kDefaultRandomizationDelayWindow_sec;
    /* PREF attributes */
    DataModel::Nullable<uint32_t> mNextChargeStartTime;
    DataModel::Nullable<uint32_t> mNextChargeTargetTime;
    DataModel::Nullable<int64_t> mNextChargeRequiredEnergy;
    DataModel::Nullable<Percent> mNextChargeTargetSoC;
    DataModel::Nullable<uint16_t> mApproximateEVEfficiency;

    /* SOC attributes */
    DataModel::Nullable<Percent> mStateOfCharge;
    DataModel::Nullable<int64_t> mBatteryCapacity;

    /* PNC attributes*/
    DataModel::Nullable<CharSpan> mVehicleID;

    EvseSession mSession = EvseSession();

    /**
     * @brief    Decides if a timer is needed based on EVSE state and sets a callback if needed
     *
     * In order to ensure the EVSE restarts charging (if enabled) after power loss
     * this should be called after the EVSE is initialised
     * (e.g. HwSetMaxHardwareCurrentLimit and HwSetCircuitCapacity have been called)
     * and the persisted attributes have been loaded, and time has been synchronised.
     *
     * If time isn't sync'd yet it will call itself back periodically (if required)
     * until time is sync'd.
     *
     * It is also called when a EnableCharging or EnableDischarging command
     * is recv'd to schedule when the EVSE should be automatically disabled based
     * on ChargingEnabledUntil / DischargingEnabledUntil expiring.
     */
    Status ScheduleCheckOnEnabledTimeout();
    bool IsEvsePluggedIn() const;

    /* Local State machine handling */
    Status CheckFaultOrDiagnostic() const;
    Status HandleChargingEnabledEvent();
    Status HandleDisabledEvent();
    Status HandleFaultRaised();
    Status HandleFaultCleared();

    Status SendEVConnectedEvent();
    Status SendEVNotDetectedEvent();
    Status SendEnergyTransferStartedEvent();
    Status SendEnergyTransferStoppedEvent(EnergyTransferStoppedReasonEnum reason);

    /**
     * @brief Helper function to work out the charge limit based on conditions and settings
     */
    Status ComputeMaxChargeCurrentLimit();
    CHIP_ERROR FindNextTarget(const BitMask<EnergyEvse::TargetDayOfWeekBitmap> dayOfWeekMap,
                              uint16_t minutesPastMidnightNow_m, uint16_t &targetTimeMinutesPastMidnight_m,
                              DataModel::Nullable<Percent> &targetSoC, DataModel::Nullable<int64_t> &addedEnergy_mWh,
                              bool bAllowTargetsInPast);
    CHIP_ERROR ComputeChargingSchedule();

    /**
     * CHIP SystemLayer timer callback; @p appState is the delegate (`this`) passed to StartTimer.
     * Signature matches chip::System::TimerCompleteCallback (second parameter is opaque user context).
     */
    static void EvseCheckTimerExpiry(System::Layer *systemLayer, void *callbackContext);

    static void ApplicationCallbackHandler(const EVSECbInfo *cb, intptr_t arg);

    /* Callback related */
    EVSECallbackWrapper mCallbacks = {.handler = nullptr, .arg = 0}; /* Wrapper to allow callbacks to be registered */
    Status HwRegisterEvseCallbackHandler(EVSECallbackFunc handler, intptr_t arg);
    Status NotifyApplicationCurrentLimitChange(int64_t maximumChargeCurrent) const;
    Status NotifyApplicationStateChange() const;
    Status NotifyApplicationChargingPreferencesChange() const;

    // -----------------------------------------------------------------
    // Internal API to allow an EVSE to change its internal state etc
    Status HwSetMaxHardwareCurrentLimit(int64_t currentmA);
    Status HwSetFault(FaultStateEnum fault);

    Status SendEvent_DetectedCard(ByteSpan uid);

    Status SendFaultEvent(FaultStateEnum newFaultState);

    EvseTargetsDelegate *GetEvseTargetsDelegate();

private:
    /** Advances @p dayOfWeekMap to the next day when the current day has no matching target. */
    void AdvanceScheduleSearchDay(BitMask<EnergyEvse::TargetDayOfWeekBitmap> &dayOfWeekMap);

    /**
     * Runs FindNextTarget for the current day; may rotate the day bitmask when nothing is found (up to 2 days).
     */
    CHIP_ERROR SearchNextChargeTargetAcrossDays(BitMask<EnergyEvse::TargetDayOfWeekBitmap> &dayOfWeekMap,
                                                uint16_t minutesPastMidnightNow_m,
                                                uint16_t &targetTimeMinutesPastMidnight_m,
                                                DataModel::Nullable<Percent> &targetSoC,
                                                DataModel::Nullable<int64_t> &addedEnergy_mWh, uint8_t &searchDay);

    /**
     * Fills next-charge start/target timestamps after a target was found (SoC path vs added-energy path).
     */
    CHIP_ERROR FillNextChargeScheduleTimes(uint8_t searchDay, uint16_t targetTimeMinutesPastMidnight_m,
                                           uint16_t minutesPastMidnightNow_m, uint32_t now_epoch_s,
                                           DataModel::Nullable<Percent> &targetSoC,
                                           DataModel::Nullable<int64_t> &addedEnergy_mWh,
                                           DataModel::Nullable<uint32_t> &startTime_epoch_s,
                                           DataModel::Nullable<uint32_t> &targetTime_epoch_s);

    /** Runs after EvseCheckTimerExpiry dispatches the CHIP timer (real work: reschedule enable-timeout). */
    void OnEvseEnableTimerExpired();

    /* private variables for controlling the hardware - these are not attributes */
    int64_t mMaxHardwareCurrentLimit = 0; /* Hardware current limit in mA */
    int64_t mCableAssemblyCurrentLimit = 0; /* Cable limit detected when cable is plugged in, in mA */
    int64_t mMaximumChargingCurrentLimitFromCommand = 0; /* Value of current maximum limit when charging enabled */
    int64_t mActualChargingCurrentLimit = 0;

    // StateEnum mHwState                              = StateEnum::kNotPluggedIn; /* Hardware state */
    /* Variables to hold State and SupplyState in case a fault is raised */
    StateEnum mStateBeforeFault = StateEnum::kUnknownEnumValue;
    SupplyStateEnum mSupplyStateBeforeFault = SupplyStateEnum::kUnknownEnumValue;

    BitMask<Feature> mFeature;

    /* Targets Delegate */
    EvseTargetsDelegate mEvseTargetsDelegate;
};

} // namespace EnergyEvse
} // namespace Clusters
} // namespace app
} // namespace chip