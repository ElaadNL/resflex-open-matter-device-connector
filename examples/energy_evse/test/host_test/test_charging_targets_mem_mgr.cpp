// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "ChargingTargetsMemMgr.h"
#include "chip_platform.h"

#include "unity.h"

#include <vector>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::EnergyEvse;
using namespace chip::app::Clusters::EnergyEvse::Structs;

static ChargingTargetStruct::Type MakeTarget(uint16_t minutes)
{
    ChargingTargetStruct::Type target{};
    target.targetTimeMinutesPastMidnight = minutes;
    return target;
}

static void test_prepare_day_schedule_clears_daily_count(void)
{
    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(0);
    mgr.AddChargingTarget(MakeTarget(60));
    TEST_ASSERT_EQUAL(1, mgr.GetNumDailyChargingTargets());

    mgr.PrepareDaySchedule(0);
    TEST_ASSERT_EQUAL(0, mgr.GetNumDailyChargingTargets());
}

static void test_prepare_day_schedule_rejects_bad_index(void)
{
    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(kEvseTargetsMaxNumberOfDays);
    TEST_ASSERT_EQUAL(0, mgr.GetNumDailyChargingTargets());
}

static void test_prepare_day_schedule_frees_previous_allocation(void)
{
    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(0);
    mgr.AddChargingTarget(MakeTarget(30));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.AllocAndCopy());
    TEST_ASSERT_NOT_NULL(mgr.GetChargingTargets());

    mgr.PrepareDaySchedule(0);
    TEST_ASSERT_NULL(mgr.GetChargingTargets());
}

static void test_add_charging_target_respects_daily_limit(void)
{
    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(1);

    for (uint16_t i = 0; i < kEvseTargetsMaxTargetsPerDay; i++) {
        mgr.AddChargingTarget(MakeTarget(static_cast<uint16_t>(i)));
    }
    TEST_ASSERT_EQUAL(kEvseTargetsMaxTargetsPerDay, mgr.GetNumDailyChargingTargets());

    mgr.AddChargingTarget(MakeTarget(999));
    TEST_ASSERT_EQUAL(kEvseTargetsMaxTargetsPerDay, mgr.GetNumDailyChargingTargets());
}

static void test_alloc_and_copy_from_daily_buffer(void)
{
    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(2);
    mgr.AddChargingTarget(MakeTarget(100));
    mgr.AddChargingTarget(MakeTarget(200));

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.AllocAndCopy());
    TEST_ASSERT_EQUAL(2, mgr.GetNumDailyChargingTargets());

    const auto *stored = mgr.GetChargingTargets();
    TEST_ASSERT_NOT_NULL(stored);
    TEST_ASSERT_EQUAL_UINT16(100, stored[0].targetTimeMinutesPastMidnight);
    TEST_ASSERT_EQUAL_UINT16(200, stored[1].targetTimeMinutesPastMidnight);
}

static void test_alloc_and_copy_empty_daily_list(void)
{
    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(0);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.AllocAndCopy());
    TEST_ASSERT_NULL(mgr.GetChargingTargets());
    TEST_ASSERT_EQUAL(0, mgr.GetNumDailyChargingTargets());
}

static void test_alloc_and_copy_from_const_list(void)
{
    const ChargingTargetStruct::Type src[] = {MakeTarget(10), MakeTarget(20), MakeTarget(30)};
    const DataModel::List<const ChargingTargetStruct::Type> list(src, 3);

    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(3);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.AllocAndCopy(list));
    TEST_ASSERT_EQUAL(3, mgr.GetNumDailyChargingTargets());

    const auto *stored = mgr.GetChargingTargets();
    TEST_ASSERT_EQUAL_UINT16(10, stored[0].targetTimeMinutesPastMidnight);
    TEST_ASSERT_EQUAL_UINT16(30, stored[2].targetTimeMinutesPastMidnight);
}

static void test_alloc_and_copy_from_decodable_list(void)
{
    DataModel::DecodableList<ChargingTargetStruct::DecodableType> list{
        ChargingTargetStruct::DecodableType{.targetTimeMinutesPastMidnight = 400},
        ChargingTargetStruct::DecodableType{.targetTimeMinutesPastMidnight = 500},
    };

    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(4);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.AllocAndCopy(list));
    TEST_ASSERT_EQUAL(2, mgr.GetNumDailyChargingTargets());
    TEST_ASSERT_EQUAL_UINT16(500, mgr.GetChargingTargets()[1].targetTimeMinutesPastMidnight);
}

static void test_alloc_and_copy_decodable_compute_size_error(void)
{
    DataModel::DecodableList<ChargingTargetStruct::DecodableType> list{};
    list.SetComputeSizeErrorForTest(CHIP_ERROR_INVALID_ARGUMENT);

    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(0);
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, mgr.AllocAndCopy(list));
}

static void test_alloc_and_copy_fails_when_out_of_memory(void)
{
    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(0);
    mgr.AddChargingTarget(MakeTarget(15));

    unit_test_platform_force_alloc_fail(true);
    TEST_ASSERT_EQUAL(CHIP_ERROR_NO_MEMORY, mgr.AllocAndCopy());
    unit_test_platform_force_alloc_fail(false);
}

static void test_alloc_and_copy_list_fails_when_out_of_memory(void)
{
    const ChargingTargetStruct::Type src[] = {MakeTarget(1)};
    const DataModel::List<const ChargingTargetStruct::Type> list(src, 1);

    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(0);

    unit_test_platform_force_alloc_fail(true);
    TEST_ASSERT_EQUAL(CHIP_ERROR_NO_MEMORY, mgr.AllocAndCopy(list));
    unit_test_platform_force_alloc_fail(false);
}

static void test_alloc_and_copy_decodable_fails_when_out_of_memory(void)
{
    DataModel::DecodableList<ChargingTargetStruct::DecodableType> list{
        ChargingTargetStruct::DecodableType{.targetTimeMinutesPastMidnight = 1},
    };

    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(0);

    unit_test_platform_force_alloc_fail(true);
    TEST_ASSERT_EQUAL(CHIP_ERROR_NO_MEMORY, mgr.AllocAndCopy(list));
    unit_test_platform_force_alloc_fail(false);
}

static void test_move_constructor_transfers_day_arrays(void)
{
    ChargingTargetsMemMgr mgr;
    mgr.PrepareDaySchedule(0);
    mgr.AddChargingTarget(MakeTarget(77));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.AllocAndCopy());

    ChargingTargetsMemMgr moved(std::move(mgr));
    TEST_ASSERT_EQUAL(1, moved.GetNumDailyChargingTargets());
    TEST_ASSERT_NOT_NULL(moved.GetChargingTargets());
    TEST_ASSERT_EQUAL_UINT16(77, moved.GetChargingTargets()[0].targetTimeMinutesPastMidnight);
    TEST_ASSERT_NULL(mgr.GetChargingTargets());
}

static void test_move_assignment_frees_existing_and_transfers(void)
{
    ChargingTargetsMemMgr dst;
    dst.PrepareDaySchedule(0);
    dst.AddChargingTarget(MakeTarget(10));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, dst.AllocAndCopy());

    ChargingTargetsMemMgr src;
    src.PrepareDaySchedule(1);
    src.AddChargingTarget(MakeTarget(88));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, src.AllocAndCopy());

    dst = std::move(src);
    TEST_ASSERT_EQUAL(1, dst.GetNumDailyChargingTargets());
    TEST_ASSERT_EQUAL_UINT16(88, dst.GetChargingTargets()[0].targetTimeMinutesPastMidnight);
    TEST_ASSERT_NULL(src.GetChargingTargets());
}

static void test_destructor_frees_multiple_day_slots(void)
{
    {
        ChargingTargetsMemMgr mgr;
        for (uint16_t day = 0; day < 3; day++) {
            mgr.PrepareDaySchedule(day);
            mgr.AddChargingTarget(MakeTarget(static_cast<uint16_t>(day * 10)));
            TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.AllocAndCopy());
        }
    }
}

static void test_sequential_day_schedule_setup(void)
{
    ChargingTargetsMemMgr mgr;

    mgr.PrepareDaySchedule(0);
    mgr.AddChargingTarget(MakeTarget(111));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.AllocAndCopy());
    TEST_ASSERT_EQUAL_UINT16(111, mgr.GetChargingTargets()[0].targetTimeMinutesPastMidnight);

    mgr.PrepareDaySchedule(1);
    TEST_ASSERT_NULL(mgr.GetChargingTargets());
    mgr.AddChargingTarget(MakeTarget(222));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, mgr.AllocAndCopy());
    TEST_ASSERT_EQUAL_UINT16(222, mgr.GetChargingTargets()[0].targetTimeMinutesPastMidnight);
}

void run_test_charging_targets_mem_mgr_tests(void)
{
    RUN_TEST(test_prepare_day_schedule_clears_daily_count);
    RUN_TEST(test_prepare_day_schedule_rejects_bad_index);
    RUN_TEST(test_prepare_day_schedule_frees_previous_allocation);
    RUN_TEST(test_add_charging_target_respects_daily_limit);
    RUN_TEST(test_alloc_and_copy_from_daily_buffer);
    RUN_TEST(test_alloc_and_copy_empty_daily_list);
    RUN_TEST(test_alloc_and_copy_from_const_list);
    RUN_TEST(test_alloc_and_copy_from_decodable_list);
    RUN_TEST(test_alloc_and_copy_decodable_compute_size_error);
    RUN_TEST(test_alloc_and_copy_fails_when_out_of_memory);
    RUN_TEST(test_alloc_and_copy_list_fails_when_out_of_memory);
    RUN_TEST(test_alloc_and_copy_decodable_fails_when_out_of_memory);
    RUN_TEST(test_move_constructor_transfers_day_arrays);
    RUN_TEST(test_move_assignment_frees_existing_and_transfers);
    RUN_TEST(test_destructor_frees_multiple_day_slots);
    RUN_TEST(test_sequential_day_schedule_setup);
}
