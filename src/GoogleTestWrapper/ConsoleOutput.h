#pragma once

#include "gtest/gtest.h"

class ConsoleOutputData;
class ConsoleOutput : public ::testing::EmptyTestEventListener 
{
private:
    class ConsoleOutputData *m_data;

public:
    static void Start();
    static void SetCompactTestOutput(bool enabled);

    ConsoleOutput();
    ~ConsoleOutput();

    virtual void OnTestProgramStart(const ::testing::UnitTest& unitTest);
    virtual void OnTestProgramEnd(const ::testing::UnitTest& unitTest);
    virtual void OnEnvironmentsSetUpEnd(const ::testing::UnitTest& unitTest);
    virtual void OnEnvironmentsTearDownEnd(const ::testing::UnitTest& unitTest);
    virtual void OnTestCaseStart(const ::testing::TestCase& testCase);
    virtual void OnTestCaseEnd(const ::testing::TestCase& testCase);
    virtual void OnTestStart(const ::testing::TestInfo& testInfo);
    virtual void OnTestPartResult(const ::testing::TestPartResult& testPartResult);
    virtual void OnTestEnd(const ::testing::TestInfo& testInfo);
};  

