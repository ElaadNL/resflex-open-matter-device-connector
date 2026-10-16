// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "chip_system_layer.h"
#include "DEMManufacturerDelegate_host.h"
#include "app/clusters/device-energy-management-server/device-energy-management-server.h"

namespace chip {
namespace app {
namespace Clusters {
namespace DeviceEnergyManagement {

class DeviceEnergyManagementDelegate : public Delegate {
public:
    DeviceEnergyManagementDelegate();

    void SetupDelegate(EndpointId id);
    void AddCustomAttributes() const;
    BitMask<Feature> mFeature;
    void AddCustomFeatures(BitMask<Feature, uint32_t> aFeature);

    void SetDeviceEnergyManagementInstance(Instance &instance);
    void SetDEMManufacturerDelegate(DEMManufacturerDelegate &deviceEnergyManagementManufacturerDelegate);

    chip::Protocols::InteractionModel::Status PowerAdjustRequest(const int64_t powerMw, const uint32_t durationS,
                                                                 AdjustmentCauseEnum cause) override;
    chip::Protocols::InteractionModel::Status CancelPowerAdjustRequest() override;
    chip::Protocols::InteractionModel::Status StartTimeAdjustRequest(const uint32_t requestedStartTimeUtc,
                                                                     AdjustmentCauseEnum cause) override;
    chip::Protocols::InteractionModel::Status PauseRequest(const uint32_t durationS,
                                                           AdjustmentCauseEnum cause) override;
    chip::Protocols::InteractionModel::Status ResumeRequest() override;
    chip::Protocols::InteractionModel::Status ModifyForecastRequest(
        const uint32_t forecastID,
        const DataModel::DecodableList<Structs::SlotAdjustmentStruct::DecodableType> &slotAdjustments,
        AdjustmentCauseEnum cause) override;
    chip::Protocols::InteractionModel::Status RequestConstraintBasedForecast(
        const DataModel::DecodableList<Structs::ConstraintsStruct::DecodableType> &constraints,
        AdjustmentCauseEnum cause) override;
    chip::Protocols::InteractionModel::Status CancelRequest() override;

    ESATypeEnum GetESAType() override;
    bool GetESACanGenerate() override;
    ESAStateEnum GetESAState() override;
    int64_t GetAbsMinPower() override;
    int64_t GetAbsMaxPower() override;
    const DataModel::Nullable<Structs::PowerAdjustCapabilityStruct::Type> &GetPowerAdjustmentCapability() override;
    const DataModel::Nullable<Structs::ForecastStruct::Type> &GetForecast() override;
    OptOutStateEnum GetOptOutState() override;

    CHIP_ERROR SetESAState(ESAStateEnum) override;
    CHIP_ERROR SetESAType(ESATypeEnum);
    CHIP_ERROR SetESACanGenerate(bool);
    CHIP_ERROR SetAbsMinPower(int64_t);
    CHIP_ERROR SetAbsMaxPower(int64_t);
    CHIP_ERROR SetPowerAdjustmentCapability(const DataModel::Nullable<Structs::PowerAdjustCapabilityStruct::Type> &);
    CHIP_ERROR SetPowerAdjustmentCapabilityPowerAdjustReason(PowerAdjustReasonEnum);
    CHIP_ERROR SetForecast(const DataModel::Nullable<Structs::ForecastStruct::Type> &);
    CHIP_ERROR SetOptOutState(OptOutStateEnum);

    uint32_t HasFeature(Feature feature) const;

private:
    void HandlePowerAdjustRequestFailure();
    static void PowerAdjustTimerExpiry(chip::DeviceLayer::SystemLayerImpl *systemLayer, void *delegate);
    void HandlePowerAdjustTimerExpiry();
    CHIP_ERROR CancelPowerAdjustRequestAndGenerateEvent(CauseEnum cause);
    CHIP_ERROR GeneratePowerAdjustEndEvent(CauseEnum cause);

    void HandlePauseRequestFailure();
    static void PauseRequestTimerExpiry(chip::DeviceLayer::SystemLayerImpl *systemLayer, void *delegate);
    void HandlePauseRequestTimerExpiry();
    CHIP_ERROR CancelPauseRequestAndGenerateEvent(CauseEnum cause);
    CHIP_ERROR GenerateResumedEvent(CauseEnum cause) const;

    bool ShouldCancelPowerAdjustForOptOut(OptOutStateEnum newValue);
    bool ShouldCancelPauseForOptOut(OptOutStateEnum newValue);
    CHIP_ERROR NormalizeForecastReasonAfterOptOut();

    Instance *mpDEMInstance = nullptr;
    DEMManufacturerDelegate *mpDEMManufacturerDelegate = nullptr;

    ESATypeEnum mEsaType;
    bool mEsaCanGenerate;
    ESAStateEnum mEsaState;
    int64_t mAbsMinPowerMw;
    int64_t mAbsMaxPowerMw;
    OptOutStateEnum mOptOutState;

    DataModel::Nullable<Structs::PowerAdjustCapabilityStruct::Type> mPowerAdjustCapabilityStruct;
    DataModel::Nullable<Structs::ForecastStruct::Type> mForecast;

    bool mPowerAdjustmentInProgress;
    uint32_t mPowerAdjustmentStartTimeUtc;
    bool mPauseRequestInProgress;
};

} // namespace DeviceEnergyManagement
} // namespace Clusters
} // namespace app
} // namespace chip
