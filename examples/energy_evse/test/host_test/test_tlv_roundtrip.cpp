// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "app-common/zap-generated/cluster-objects.h"
#include "app/clusters/energy-evse-server/energy-evse-server.h"
#include "chip_support.h"
#include "lib/core/TLV.h"
#include "lib/support/ScopedBuffer.h"

#include "unity.h"

#include <cstdio>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters::EnergyEvse;

enum class TargetEntryTag : uint8_t {
    kDayOfWeek = 2,
    kChargingTargetsList = 3,
    kChargingTargetsStruct = 4,
    kTargetTime = 5,
    kTargetSoC = 6,
};

static void test_tlv_target_schedule_roundtrip(void)
{
    uint16_t total = 4096;
    ScopedMemoryBuffer<uint8_t> writerBuffer;
    TEST_ASSERT_TRUE(writerBuffer.Calloc(total));

    TLV::ScopedBufferTLVWriter writer(std::move(writerBuffer), total);
    TLV::TLVType arrayType;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, writer.StartContainer(TLV::AnonymousTag(), TLV::kTLVType_Array, arrayType));

    TLV::TLVType scheduleType;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, writer.StartContainer(TLV::AnonymousTag(), TLV::kTLVType_Structure, scheduleType));
    TEST_ASSERT_EQUAL(
        CHIP_NO_ERROR,
        writer.Put(TLV::ContextTag(TargetEntryTag::kDayOfWeek), BitMask<TargetDayOfWeekBitmap>(kAllTargetDaysMask)));

    TLV::TLVType listType;
    TEST_ASSERT_EQUAL(
        CHIP_NO_ERROR,
        writer.StartContainer(TLV::ContextTag(TargetEntryTag::kChargingTargetsList), TLV::kTLVType_List, listType));

    TLV::TLVType targetType;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR,
                      writer.StartContainer(TLV::ContextTag(TargetEntryTag::kChargingTargetsStruct),
                                            TLV::kTLVType_Structure, targetType));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR,
                      writer.Put(TLV::ContextTag(TargetEntryTag::kTargetTime), static_cast<uint16_t>(600)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, writer.Put(TLV::ContextTag(TargetEntryTag::kTargetSoC), static_cast<uint8_t>(90)));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, writer.EndContainer(targetType));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, writer.EndContainer(listType));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, writer.EndContainer(scheduleType));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, writer.EndContainer(arrayType));

    ScopedMemoryBuffer<uint8_t> finalizedBuffer;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, writer.Finalize(finalizedBuffer));

    const uint16_t len = static_cast<uint16_t>(writer.GetLengthWritten());
    TLV::ScopedBufferTLVReader reader(std::move(finalizedBuffer), len);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.Next(TLV::kTLVType_Array, TLV::AnonymousTag()));
    TLV::TLVType readArrayType;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.EnterContainer(readArrayType));

    CHIP_ERROR err = reader.Next(TLV::kTLVType_Structure, TLV::AnonymousTag());
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, err);

    TLV::TLVType readScheduleType;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.EnterContainer(readScheduleType));

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.Next(TLV::ContextTag(TargetEntryTag::kDayOfWeek)));

    BitMask<TargetDayOfWeekBitmap> day{};
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.Get(day));
    TEST_ASSERT_EQUAL(kAllTargetDaysMask, day.Raw());

    err = reader.Next(TLV::kTLVType_List, TLV::ContextTag(TargetEntryTag::kChargingTargetsList));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, err);

    TLV::TLVType readListType;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.EnterContainer(readListType));

    err = reader.Next(TLV::kTLVType_Structure, TLV::ContextTag(TargetEntryTag::kChargingTargetsStruct));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, err);

    TLV::TLVType readTargetType;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.EnterContainer(readTargetType));

    uint16_t minutes = 0;
    while ((err = reader.Next()) == CHIP_NO_ERROR) {
        const auto tag = reader.GetTag();
        if (tag == TLV::ContextTag(TargetEntryTag::kTargetTime)) {
            TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.Get(minutes));
        }
    }
    TEST_ASSERT_EQUAL(CHIP_END_OF_TLV, err);
    TEST_ASSERT_EQUAL(600u, minutes);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.ExitContainer(readTargetType));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.ExitContainer(readListType));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.ExitContainer(readScheduleType));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.ExitContainer(readArrayType));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, reader.VerifyEndOfContainer());
}

void run_test_tlv_roundtrip_tests(void)
{
    RUN_TEST(test_tlv_target_schedule_roundtrip);
}
