#!/bin/bash
# SPDX-FileCopyrightText: ElaadNL
# SPDX-License-Identifier: Apache-2.0


pushd examples/energy_evse/test/host_test > /dev/null
# Build the tests
if [ -d "build" ]; then
    echo "Cleaning previous build artifacts..."
    rm -rf build
fi

echo "Building unit tests with coverage instrumentation..."
mkdir build && cd build

echo "Running CMake configuration..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

echo "Compiling tests..."
if ! make -j$(nproc); then
    echo "Build failed. Please check the output for errors."
    exit 1
fi

echo "Running unit tests..."
if ./run_tests; then
    echo "All tests passed successfully."
else
    echo "Some tests failed. Please check the output for details."
    exit 1
fi

echo "Generating coverage report..."
report_date=$(date +%Y%m%d)

if ! gcovr . --sonarqube "sonar-coverage-${report_date}.xml" -r ../../../ --gcov-executable "xcrun llvm-cov gcov"; then
    echo "Coverage report generation failed. Please check the output for errors."
    exit 1
fi

echo "Coverage report generated successfully: sonar-coverage-${report_date}.xml"