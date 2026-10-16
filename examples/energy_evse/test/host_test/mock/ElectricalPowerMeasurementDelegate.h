// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app/clusters/electrical-power-measurement-server/electrical-power-measurement-server.h"
#include "app/util/af-types.h"
#include "chip_support.h"

namespace chip {
namespace app {
namespace Clusters {
namespace ElectricalPowerMeasurement {

class ElectricalPowerMeasurementDelegate : public Delegate {
public:
    void SetupDelegate(EndpointId id);
    EndpointId GetEndpointId() const;
    void AddCustomAttributes() const;
    void AddCustomFeatures(Feature aFeature);
    void LateSetupAfterMatter() const;

    ~ElectricalPowerMeasurementDelegate() override = default;

    static constexpr uint8_t kMaxNumberOfMeasurementTypes = 14;
    static constexpr uint8_t kDefaultNumberOfMeasurementTypes = 1;

    PowerModeEnum GetPowerMode() override;
    uint8_t GetNumberOfMeasurementTypes() override;

    CHIP_ERROR StartAccuracyRead() override;
    CHIP_ERROR GetAccuracyByIndex(uint8_t, Structs::MeasurementAccuracyStruct::Type &) override;
    CHIP_ERROR EndAccuracyRead() override;

    CHIP_ERROR StartRangesRead() override;
    CHIP_ERROR GetRangeByIndex(uint8_t, Structs::MeasurementRangeStruct::Type &) override;
    CHIP_ERROR EndRangesRead() override;

    CHIP_ERROR StartHarmonicCurrentsRead() override;
    CHIP_ERROR GetHarmonicCurrentsByIndex(uint8_t, Structs::HarmonicMeasurementStruct::Type &) override;
    CHIP_ERROR EndHarmonicCurrentsRead() override;

    CHIP_ERROR StartHarmonicPhasesRead() override;
    CHIP_ERROR GetHarmonicPhasesByIndex(uint8_t, Structs::HarmonicMeasurementStruct::Type &) override;
    CHIP_ERROR EndHarmonicPhasesRead() override;

    DataModel::Nullable<int64_t> GetVoltage() override;
    DataModel::Nullable<int64_t> GetActiveCurrent() override;
    DataModel::Nullable<int64_t> GetReactiveCurrent() override;
    DataModel::Nullable<int64_t> GetApparentCurrent() override;
    DataModel::Nullable<int64_t> GetActivePower() override;
    DataModel::Nullable<int64_t> GetReactivePower() override;
    DataModel::Nullable<int64_t> GetApparentPower() override;
    DataModel::Nullable<int64_t> GetRMSVoltage() override;
    DataModel::Nullable<int64_t> GetRMSCurrent() override;
    DataModel::Nullable<int64_t> GetRMSPower() override;
    DataModel::Nullable<int64_t> GetFrequency() override;
    DataModel::Nullable<int64_t> GetPowerFactor() override;
    DataModel::Nullable<int64_t> GetNeutralCurrent() override;

    CHIP_ERROR SetPowerMode(PowerModeEnum);
    CHIP_ERROR SetVoltage(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetActiveCurrent(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetReactiveCurrent(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetApparentCurrent(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetActivePower(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetReactivePower(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetApparentPower(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetRMSVoltage(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetRMSCurrent(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetRMSPower(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetFrequency(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetPowerFactor(const DataModel::Nullable<int64_t> &);
    CHIP_ERROR SetNeutralCurrent(const DataModel::Nullable<int64_t> &);

    BitMask<Feature> mFeature;

private:
    PowerModeEnum mPowerMode = PowerModeEnum::kAc;
    DataModel::Nullable<int64_t> mVoltage;
    DataModel::Nullable<int64_t> mActiveCurrent;
    DataModel::Nullable<int64_t> mReactiveCurrent;
    DataModel::Nullable<int64_t> mApparentCurrent;
    DataModel::Nullable<int64_t> mActivePower;
    DataModel::Nullable<int64_t> mReactivePower;
    DataModel::Nullable<int64_t> mApparentPower;
    DataModel::Nullable<int64_t> mRMSVoltage;
    DataModel::Nullable<int64_t> mRMSCurrent;
    DataModel::Nullable<int64_t> mRMSPower;
    DataModel::Nullable<int64_t> mFrequency;
    DataModel::Nullable<int64_t> mPowerFactor;
    DataModel::Nullable<int64_t> mNeutralCurrent;
};

class ElectricalPowerMeasurementInstance : public Instance {
public:
    ElectricalPowerMeasurementInstance(EndpointId aEndpointId, ElectricalPowerMeasurementDelegate &aDelegate,
                                       Feature aFeature, OptionalAttributes aOptionalAttributes)
        : Instance(aEndpointId, aDelegate, BitMask<Feature>(aFeature), BitMask<OptionalAttributes>(aOptionalAttributes))
    {
        mDelegate = &aDelegate;
    }

    ElectricalPowerMeasurementInstance(const ElectricalPowerMeasurementInstance &) = delete;
    ElectricalPowerMeasurementInstance(const ElectricalPowerMeasurementInstance &&) = delete;
    ElectricalPowerMeasurementInstance &operator=(const ElectricalPowerMeasurementInstance &) = delete;

    CHIP_ERROR InitializeCluster();
    void ShutdownCluster();

    ElectricalPowerMeasurementDelegate *GetDelegate();

private:
    ElectricalPowerMeasurementDelegate *mDelegate = nullptr;
};

} // namespace ElectricalPowerMeasurement
} // namespace Clusters
} // namespace app
} // namespace chip
