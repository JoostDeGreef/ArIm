#pragma once

#include "gtest/gtest.h"

class ConsoleOutputData;
class ConsoleOutput : public ::testing::EmptyTestEventListener 
{
private:
    class ConsoleOutputData *m_data;

public:
    static void Start();

    ConsoleOutput();
    ~ConsoleOutput();

    virtual void OnTestProgramStart(const ::testing::UnitTest& unit_test);
    virtual void OnTestProgramEnd(const ::testing::UnitTest& unit_test);
    virtual void OnEnvironmentsSetUpEnd(const ::testing::UnitTest& unit_test);
    virtual void OnEnvironmentsTearDownEnd(const ::testing::UnitTest& unit_test);
    virtual void OnTestCaseStart(const ::testing::TestCase& test_case);
    virtual void OnTestCaseEnd(const ::testing::TestCase& test_case);
    virtual void OnTestStart(const ::testing::TestInfo& test_info);
    virtual void OnTestPartResult(const ::testing::TestPartResult& test_part_result);
    virtual void OnTestEnd(const ::testing::TestInfo& test_info);
};  

