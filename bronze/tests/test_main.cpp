/**
 * @file test_main.cpp
 * @brief Main entry point for the HM11 driver unit tests
 *
 * This file is part of the HM11 driver C++ migration project.
 * It provides the main entry point for running all unit tests.
 * It configures Google Test with custom output and failure behavior.
 *
 * @copyright Copyright (C) 2025
 */

#include <gtest/gtest.h>

#include <iostream>
#include <string>

/**
 * @brief Custom test event listener for enhanced test output
 *
 * This listener provides more detailed output for test failures and
 * prints a summary of all tests after they have completed.
 */
class HM11TestListener : public ::testing::EmptyTestEventListener {
  public:
    void OnTestProgramStart(const ::testing::UnitTest& /* unit_test */) override {
        std::cout << "\n"
                  << "==========================================================\n"
                  << " HM11 Driver Test Suite\n"
                  << "==========================================================\n"
                  << "\n";
    }

    void OnTestStart(const ::testing::TestInfo& test_info) override {
        std::cout << "[ RUN      ] " << test_info.test_suite_name() << "." << test_info.name() << "\n";
    }

    void OnTestPartResult(const ::testing::TestPartResult& result) override {
        if (result.failed()) {
            std::cout << "[ ERROR    ] " << result.file_name() << ":" << result.line_number() << "\n"
                      << result.summary() << "\n";
        }
    }

    void OnTestEnd(const ::testing::TestInfo& test_info) override {
        if (test_info.result()->Passed()) {
            std::cout << "[       OK ] " << test_info.test_suite_name() << "." << test_info.name() << " ("
                      << test_info.result()->elapsed_time() << " ms)\n";
        } else {
            std::cout << "[  FAILED  ] " << test_info.test_suite_name() << "." << test_info.name() << " ("
                      << test_info.result()->elapsed_time() << " ms)\n";
        }
    }

    void OnTestProgramEnd(const ::testing::UnitTest& unit_test) override {
        std::cout << "\n"
                  << "==========================================================\n";

        if (unit_test.Failed()) {
            std::cout << " TEST FAILURE SUMMARY\n"
                      << "==========================================================\n";
            for (int i = 0; i < unit_test.total_test_suite_count(); ++i) {
                const auto* test_suite = unit_test.GetTestSuite(i);
                for (int j = 0; j < test_suite->total_test_count(); ++j) {
                    const auto* test_info = test_suite->GetTestInfo(j);
                    if (!test_info->result()->Passed()) {
                        std::cout << "[  FAILED  ] " << test_info->test_suite_name() << "." << test_info->name()
                                  << "\n";
                    }
                }
            }
            std::cout << "\n";
        }

        std::cout << " TEST EXECUTION SUMMARY\n"
                  << "==========================================================\n"
                  << " Total Tests: " << unit_test.total_test_count() << "\n"
                  << " Passed     : " << unit_test.successful_test_count() << "\n"
                  << " Failed     : " << unit_test.failed_test_count() << "\n"
                  << " Disabled   : " << unit_test.disabled_test_count() << "\n"
                  << " Time       : " << unit_test.elapsed_time() << " ms\n"
                  << "==========================================================\n"
                  << (unit_test.Passed() ? "ALL TESTS PASSED\n" : "SOME TESTS FAILED\n")
                  << "==========================================================\n";
    }
};

/**
 * @brief Configure Google Test with custom options
 *
 * This function configures Google Test with custom options for output
 * format, test filtering, and behavior on test failures.
 *
 * @param argc Number of command-line arguments
 * @param argv Command-line arguments
 */
void configure_gtest(int argc, char** argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // Remove the default listener
    auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());

    // Add our custom listener
    listeners.Append(new HM11TestListener());

    // Set other test behavior
    if (::testing::GTEST_FLAG(break_on_failure)) {
        std::cout << "Note: Break-on-failure is enabled\n";
    }
}

/**
 * @brief Main entry point for the test program
 *
 * @param argc Number of command-line arguments
 * @param argv Command-line arguments
 * @return int Test execution status (0 for success, non-zero for failure)
 */
int main(int argc, char** argv) {
    // Configure Google Test
    configure_gtest(argc, argv);

    // Run all tests
    return RUN_ALL_TESTS();
}