# SPDX-FileCopyrightText: ElaadNL
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)
project(host_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

add_compile_options(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=)
add_compile_options(--coverage -O0 -g -fprofile-arcs -ftest-coverage)
add_link_options(--coverage)

include_directories(.)
include_directories(./mock)
include_directories(./unit_sources)
include_directories(../../main)
include_directories(../../main/driver/unit)
include_directories(../../main/driver/ev_charger/include)

file(GLOB LOCAL_SOURCES *.cpp *.c)
file(GLOB UNIT_DRIVER_SOURCES ../../main/driver/unit/*.cpp)
file(GLOB MOCK_SOURCES mock/*.cpp unit_sources/*.cpp)
list(REMOVE_ITEM MOCK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/mock/matterManager.cpp")

set(EV_DRIVER_SOURCES
    ../../main/driver/ev_charger/app_cmd.cpp
    ../../main/driver/ev_charger/charger_uiux_handler.cpp
    ../../main/driver/ev_charger/chargerManager.cpp
    ../../main/driver/ev_charger/hardwareControlInterface.cpp
    ../../main/driver/ev_charger/EnergyTimeUtils.cpp
    ../../main/driver/ev_charger/TimeSync.cpp
    ../../main/driver/ev_charger/PowerTopologyDelegate.cpp
    ../../main/driver/ev_charger/energy-evse-mode.cpp
    ../../main/driver/ev_charger/ESP32DeviceInstanceInfoProvider.cpp
    ../../main/driver/ev_charger/ChargingTargetsMemMgr.cpp
    ../../main/driver/ev_charger/DeviceEnergyManagementDelegateImpl.cpp
    ../../main/driver/ev_charger/ElectricalPowerMeasurementDelegate.cpp
    ../../main/driver/ev_charger/EnergyEvseDelegateImpl.cpp
    ../../main/driver/ev_charger/EnergyEvseTargetsStore.cpp
    ../../main/driver/ev_charger/matterManager.cpp
)

set(MAIN_SOURCES
    ../../main/app_main.cpp
    ../../main/nvs_helpers.cpp
)

add_executable(run_tests
    ${LOCAL_SOURCES}
    ${UNIT_DRIVER_SOURCES}
    ${EV_DRIVER_SOURCES}
    ${MAIN_SOURCES}
    ${MOCK_SOURCES}
)

target_compile_definitions(run_tests PRIVATE UNIT_TEST)
target_link_libraries(run_tests)
