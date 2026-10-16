# SPDX-FileCopyrightText: ElaadNL
# SPDX-License-Identifier: Apache-2.0

set(SRC_DIRS_LIST "."
    "driver/ev_charger/"
    "driver/unit/"
)
set(INCLUDE_DIRS_LIST "."
    "driver/ev_charger/"
    "driver/ev_charger/include/"
    "driver/unit/"
)

idf_component_register(SRC_DIRS          ${SRC_DIRS_LIST}
                      INCLUDE_DIRS       ${INCLUDE_DIRS_LIST}
                      PRIV_INCLUDE_DIRS  "." "${ESP_MATTER_PATH}/examples/common/utils")

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")

idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security;-Wformat=0;-Wunused-variable;-Wunused-function" APPEND)
