// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "ElectricalPowerMeasurementDelegate.h"
#include "matterManager.h"

#include "unity.h"

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::ElectricalPowerMeasurement;
using namespace chip::app::DataModel;
using CT::Charger::MatterManager;

namespace {

void reset_harness(void)
{
    MatterManager::GetInstance().isConnected = true;
    MatterManager::ReportAttributeCallCount() = 0;
}

DataModel::Nullable<int64_t> make_value(int64_t v)
{
    return DataModel::MakeNullable(v);
}

} // namespace

static void test_setup_delegate_and_features(void)
{
    reset_harness();
    ElectricalPowerMeasurementDelegate delegate;

    delegate.SetupDelegate(5);
    TEST_ASSERT_EQUAL(5, delegate.GetEndpointId());
    delegate.AddCustomFeatures(Feature::kAlternatingCurrent);
    TEST_ASSERT_TRUE(delegate.mFeature.Has(Feature::kAlternatingCurrent));
    delegate.LateSetupAfterMatter();
}

static void test_measurement_type_count(void)
{
    reset_harness();
    ElectricalPowerMeasurementDelegate delegate;
    TEST_ASSERT_EQUAL_UINT8(3, delegate.GetNumberOfMeasurementTypes());
}

static void test_power_mode_set_and_constraint(void)
{
    reset_harness();
    ElectricalPowerMeasurementDelegate delegate;
    delegate.SetupDelegate(1);

    const int before = MatterManager::ReportAttributeCallCount();
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetPowerMode(PowerModeEnum::kDc));
    TEST_ASSERT_EQUAL(static_cast<int>(PowerModeEnum::kDc), static_cast<int>(delegate.GetPowerMode()));
    TEST_ASSERT_TRUE(MatterManager::ReportAttributeCallCount() > before);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetPowerMode(PowerModeEnum::kDc));
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.SetPowerMode(PowerModeEnum::kUnknownEnumValue));
}

static void test_accuracy_read_cycle(void)
{
    reset_harness();
    ElectricalPowerMeasurementDelegate delegate;

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.StartAccuracyRead());

    Structs::MeasurementAccuracyStruct::Type accuracy{};
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetAccuracyByIndex(0, accuracy));
    TEST_ASSERT_EQUAL(static_cast<int>(MeasurementTypeEnum::kActivePower), static_cast<int>(accuracy.measurementType));
    TEST_ASSERT_TRUE(accuracy.measured);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetAccuracyByIndex(1, accuracy));
    TEST_ASSERT_EQUAL(static_cast<int>(MeasurementTypeEnum::kActiveCurrent),
                      static_cast<int>(accuracy.measurementType));

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetAccuracyByIndex(2, accuracy));
    TEST_ASSERT_EQUAL(static_cast<int>(MeasurementTypeEnum::kVoltage), static_cast<int>(accuracy.measurementType));

    TEST_ASSERT_EQUAL(CHIP_ERROR_PROVIDER_LIST_EXHAUSTED, delegate.GetAccuracyByIndex(3, accuracy));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.EndAccuracyRead());
}

static void test_ranges_read_cycle(void)
{
    reset_harness();
    ElectricalPowerMeasurementDelegate delegate;

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.StartRangesRead());
    Structs::MeasurementRangeStruct::Type range{};
    TEST_ASSERT_EQUAL(CHIP_ERROR_PROVIDER_LIST_EXHAUSTED, delegate.GetRangeByIndex(0, range));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.EndRangesRead());
}

static void test_harmonic_currents_read_cycle(void)
{
    reset_harness();
    ElectricalPowerMeasurementDelegate delegate;

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.StartHarmonicCurrentsRead());

    Structs::HarmonicMeasurementStruct::Type harmonic{};
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetHarmonicCurrentsByIndex(0, harmonic));
    TEST_ASSERT_EQUAL(1u, harmonic.order);
    TEST_ASSERT_FALSE(harmonic.measurement.IsNull());
    TEST_ASSERT_EQUAL(100000, harmonic.measurement.Value());

    TEST_ASSERT_EQUAL(CHIP_ERROR_PROVIDER_LIST_EXHAUSTED, delegate.GetHarmonicCurrentsByIndex(1, harmonic));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.EndHarmonicCurrentsRead());
}

static void test_harmonic_phases_read_cycle(void)
{
    reset_harness();
    ElectricalPowerMeasurementDelegate delegate;

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.StartHarmonicPhasesRead());

    Structs::HarmonicMeasurementStruct::Type harmonic{};
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetHarmonicPhasesByIndex(0, harmonic));
    TEST_ASSERT_EQUAL(1u, harmonic.order);

    TEST_ASSERT_EQUAL(CHIP_ERROR_PROVIDER_LIST_EXHAUSTED, delegate.GetHarmonicPhasesByIndex(1, harmonic));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.EndHarmonicPhasesRead());
}

static void test_set_measurement_attributes_report_changes(void)
{
    reset_harness();
    ElectricalPowerMeasurementDelegate delegate;
    delegate.SetupDelegate(2);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetVoltage(make_value(230000)));
    TEST_ASSERT_EQUAL(230000, delegate.GetVoltage().Value());

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetActiveCurrent(make_value(16000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetReactiveCurrent(make_value(1000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetApparentCurrent(make_value(17000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetActivePower(make_value(3600000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetReactivePower(make_value(500000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetApparentPower(make_value(3700000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetRMSVoltage(make_value(230000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetRMSCurrent(make_value(16000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetRMSPower(make_value(3600000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetFrequency(make_value(50000)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetPowerFactor(make_value(9800)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetNeutralCurrent(make_value(500)));

    TEST_ASSERT_EQUAL(230000, delegate.GetVoltage().Value());
    TEST_ASSERT_EQUAL(16000, delegate.GetActiveCurrent().Value());
    TEST_ASSERT_EQUAL(1000, delegate.GetReactiveCurrent().Value());
    TEST_ASSERT_EQUAL(17000, delegate.GetApparentCurrent().Value());
    TEST_ASSERT_EQUAL(3600000, delegate.GetActivePower().Value());
    TEST_ASSERT_EQUAL(500000, delegate.GetReactivePower().Value());
    TEST_ASSERT_EQUAL(3700000, delegate.GetApparentPower().Value());
    TEST_ASSERT_EQUAL(230000, delegate.GetRMSVoltage().Value());
    TEST_ASSERT_EQUAL(16000, delegate.GetRMSCurrent().Value());
    TEST_ASSERT_EQUAL(3600000, delegate.GetRMSPower().Value());
    TEST_ASSERT_EQUAL(50000, delegate.GetFrequency().Value());
    TEST_ASSERT_EQUAL(9800, delegate.GetPowerFactor().Value());
    TEST_ASSERT_EQUAL(500, delegate.GetNeutralCurrent().Value());

    TEST_ASSERT_TRUE(MatterManager::ReportAttributeCallCount() >= 12);

    const int before = MatterManager::ReportAttributeCallCount();
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.SetVoltage(make_value(230000)));
    TEST_ASSERT_EQUAL(before, MatterManager::ReportAttributeCallCount());
}

static void test_instance_init_and_shutdown(void)
{
    reset_harness();
    ElectricalPowerMeasurementDelegate delegate;
    ElectricalPowerMeasurementInstance instance(3, delegate, Feature::kAlternatingCurrent,
                                                OptionalAttributes::kOptionalAttributeVoltage);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, instance.InitializeCluster());
    instance.ShutdownCluster();
    TEST_ASSERT_NOT_NULL(instance.GetDelegate());
}

void run_test_electrical_power_measurement_delegate_tests(void)
{
    RUN_TEST(test_setup_delegate_and_features);
    RUN_TEST(test_measurement_type_count);
    RUN_TEST(test_power_mode_set_and_constraint);
    RUN_TEST(test_accuracy_read_cycle);
    RUN_TEST(test_ranges_read_cycle);
    RUN_TEST(test_harmonic_currents_read_cycle);
    RUN_TEST(test_harmonic_phases_read_cycle);
    RUN_TEST(test_set_measurement_attributes_report_changes);
    RUN_TEST(test_instance_init_and_shutdown);
}
