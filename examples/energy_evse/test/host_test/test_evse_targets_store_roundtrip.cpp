// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "EnergyEvseTargetsStore.h"
#include "app/server/Server.h"
#include "chip_system_layer.h"

#include "unity.h"

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters::EnergyEvse;

static void test_set_targets_save_load_roundtrip(void)
{
    Server::GetInstance().GetInMemoryStorage().ClearForTest();

    EvseTargetsDelegate targets;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, targets.Init(&Server::GetInstance().GetPersistentStorage()));

    Structs::ChargingTargetScheduleStruct::DecodableType schedule{};
    schedule.dayOfWeekForSequence = BitMask<TargetDayOfWeekBitmap>(kAllTargetDaysMask);
    Structs::ChargingTargetStruct::DecodableType target{};
    target.targetTimeMinutesPastMidnight = 600;
    target.targetSoC.SetValue(90);
    schedule.chargingTargets.SetItemsForTest({target});

    DataModel::DecodableList<Structs::ChargingTargetScheduleStruct::DecodableType> incoming;
    incoming.SetItemsForTest({schedule});

    size_t incomingCount = 0;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, incoming.ComputeSize(&incomingCount));
    TEST_ASSERT_EQUAL(1u, incomingCount);

    const CHIP_ERROR setErr = targets.SetTargets(incoming);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, setErr);
    TEST_ASSERT_EQUAL(1u, targets.GetTargets().size());

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, targets.ClearTargets());
    TEST_ASSERT_EQUAL(0u, targets.GetTargets().size());
}

void run_test_evse_targets_store_roundtrip_tests(void)
{
    RUN_TEST(test_set_targets_save_load_roundtrip);
}
