// SPDX-FileCopyrightText: ElaadNL
// SPDX-License-Identifier: Apache-2.0

#include "energy_evse_mode_host.h"

#include "unity.h"

#include <cstring>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters::EnergyEvseMode;
using ModeTagStructType = chip::app::Clusters::detail::Structs::ModeTagStruct::Type;
using ModeTagList = chip::app::DataModel::List<ModeTagStructType>;

static void test_init_returns_ok(void)
{
    EnergyEvseModeDelegate delegate;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.Init());
}

static void test_handle_change_to_mode_success(void)
{
    EnergyEvseModeDelegate delegate;
    ModeBase::Commands::ChangeToModeResponse::Type response{};
    delegate.HandleChangeToMode(kModeSolarCharging, response);
    TEST_ASSERT_EQUAL_UINT8(0, response.status);
}

static void test_get_mode_value_by_index(void)
{
    EnergyEvseModeDelegate delegate;
    uint8_t mode = 0xFF;
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeValueByIndex(0, mode));
    TEST_ASSERT_EQUAL_UINT8(kModeManual, mode);
}

static void test_get_mode_value_all_indices(void)
{
    EnergyEvseModeDelegate delegate;
    uint8_t mode = 0;

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeValueByIndex(1, mode));
    TEST_ASSERT_EQUAL_UINT8(kModeTimeOfUse, mode);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeValueByIndex(2, mode));
    TEST_ASSERT_EQUAL_UINT8(kModeSolarCharging, mode);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeValueByIndex(3, mode));
    TEST_ASSERT_EQUAL_UINT8(kModeTimeOfUseAndSolarCharging, mode);
}

static void test_get_mode_value_exhausted(void)
{
    EnergyEvseModeDelegate delegate;
    uint8_t mode = 0;
    TEST_ASSERT_EQUAL(CHIP_ERROR_PROVIDER_LIST_EXHAUSTED, delegate.GetModeValueByIndex(99, mode));
}

static MutableCharSpan make_label_span(char *buf, size_t size)
{
    MutableCharSpan label;
    label.data = buf;
    label.size = size;
    return label;
}

static void test_get_mode_label_all_indices(void)
{
    EnergyEvseModeDelegate delegate;
    char buf[64] = {};
    MutableCharSpan label = make_label_span(buf, sizeof(buf));

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeLabelByIndex(0, label));
    TEST_ASSERT_EQUAL_MEMORY("Manual", buf, 6);

    std::memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeLabelByIndex(1, label));
    TEST_ASSERT_EQUAL_MEMORY("Auto-scheduled", buf, 14);

    std::memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeLabelByIndex(2, label));
    TEST_ASSERT_EQUAL_MEMORY("Solar", buf, 5);

    std::memset(buf, 0, sizeof(buf));
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeLabelByIndex(3, label));
    TEST_ASSERT_EQUAL_MEMORY("Auto-scheduled with Solar charging", buf, 35);
}

static void test_get_mode_label_exhausted(void)
{
    EnergyEvseModeDelegate delegate;
    char buf[16] = {};
    MutableCharSpan label = make_label_span(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(CHIP_ERROR_PROVIDER_LIST_EXHAUSTED, delegate.GetModeLabelByIndex(99, label));
}

static void test_get_mode_label_buffer_too_small(void)
{
    EnergyEvseModeDelegate delegate;
    char buf[4] = {};
    MutableCharSpan label = make_label_span(buf, sizeof(buf));
    TEST_ASSERT_EQUAL(CHIP_ERROR_BUFFER_TOO_SMALL, delegate.GetModeLabelByIndex(3, label));
}

static void test_get_mode_tags_single_tag_modes(void)
{
    EnergyEvseModeDelegate delegate;
    ModeTagStructType tagBuf[2] = {};
    ModeTagList tags(tagBuf, 2);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeTagsByIndex(0, tags));
    TEST_ASSERT_EQUAL(1, tags.size());
    TEST_ASSERT_EQUAL_UINT8(0, tagBuf[0].value);

    tags = ModeTagList(tagBuf, 2);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeTagsByIndex(1, tags));
    TEST_ASSERT_EQUAL(1, tags.size());
    TEST_ASSERT_EQUAL_UINT8(1, tagBuf[0].value);

    tags = ModeTagList(tagBuf, 2);
    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeTagsByIndex(2, tags));
    TEST_ASSERT_EQUAL(1, tags.size());
    TEST_ASSERT_EQUAL_UINT8(2, tagBuf[0].value);
}

static void test_get_mode_tags_dual_tag_mode(void)
{
    EnergyEvseModeDelegate delegate;
    ModeTagStructType tagBuf[2] = {};
    ModeTagList tags(tagBuf, 2);

    TEST_ASSERT_EQUAL(CHIP_NO_ERROR, delegate.GetModeTagsByIndex(3, tags));
    TEST_ASSERT_EQUAL(2, tags.size());
    TEST_ASSERT_EQUAL_UINT8(1, tagBuf[0].value);
    TEST_ASSERT_EQUAL_UINT8(2, tagBuf[1].value);
}

static void test_get_mode_tags_exhausted(void)
{
    EnergyEvseModeDelegate delegate;
    ModeTagStructType tagBuf[1] = {};
    ModeTagList tags(tagBuf, 1);
    TEST_ASSERT_EQUAL(CHIP_ERROR_PROVIDER_LIST_EXHAUSTED, delegate.GetModeTagsByIndex(99, tags));
}

static void test_get_mode_tags_buffer_too_small(void)
{
    EnergyEvseModeDelegate delegate;
    ModeTagStructType tagBuf[1] = {};
    ModeTagList tags(tagBuf, 1);
    TEST_ASSERT_EQUAL(CHIP_ERROR_INVALID_ARGUMENT, delegate.GetModeTagsByIndex(3, tags));
}

void run_test_energy_evse_mode_tests(void)
{
    RUN_TEST(test_init_returns_ok);
    RUN_TEST(test_handle_change_to_mode_success);
    RUN_TEST(test_get_mode_value_by_index);
    RUN_TEST(test_get_mode_value_all_indices);
    RUN_TEST(test_get_mode_value_exhausted);
    RUN_TEST(test_get_mode_label_all_indices);
    RUN_TEST(test_get_mode_label_exhausted);
    RUN_TEST(test_get_mode_label_buffer_too_small);
    RUN_TEST(test_get_mode_tags_single_tag_modes);
    RUN_TEST(test_get_mode_tags_dual_tag_mode);
    RUN_TEST(test_get_mode_tags_exhausted);
    RUN_TEST(test_get_mode_tags_buffer_too_small);
}
