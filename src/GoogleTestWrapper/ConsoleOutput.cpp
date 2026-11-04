#include <vector>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>

#if defined(WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include <cstdarg>

#include "ConsoleOutput.h"

using namespace ::testing;
using namespace std;

namespace 
{
	bool compactTestOutput = false;

	class FilteringResultPrinter : public ::testing::EmptyTestEventListener
	{
	public:
		explicit FilteringResultPrinter(::testing::TestEventListener* printer)
			: m_printer(printer)
		{}

		~FilteringResultPrinter() override { delete m_printer; }

		void OnTestProgramStart(const UnitTest& unitTest) override { m_printer->OnTestProgramStart(unitTest); }
		void OnTestIterationStart(const UnitTest& unitTest, int iteration) override { m_printer->OnTestIterationStart(unitTest, iteration); }
		void OnEnvironmentsSetUpStart(const UnitTest& unitTest) override { m_printer->OnEnvironmentsSetUpStart(unitTest); }
		void OnEnvironmentsSetUpEnd(const UnitTest& unitTest) override { m_printer->OnEnvironmentsSetUpEnd(unitTest); }
		void OnTestSuiteStart(const TestSuite& testSuite) override { m_printer->OnTestSuiteStart(testSuite); }
#ifndef GTEST_REMOVE_LEGACY_TEST_CASEAPI_
		void OnTestCaseStart(const TestCase& testCase) override { m_printer->OnTestCaseStart(testCase); }
#endif
		void OnTestStart(const TestInfo& testInfo) override
		{
			if(!compactTestOutput) m_printer->OnTestStart(testInfo);
		}
		void OnTestDisabled(const TestInfo& testInfo) override { m_printer->OnTestDisabled(testInfo); }
		void OnTestPartResult(const TestPartResult& result) override { m_printer->OnTestPartResult(result); }
		void OnTestEnd(const TestInfo& testInfo) override
		{
			if(!compactTestOutput || !testInfo.result()->Passed()) m_printer->OnTestEnd(testInfo);
		}
		void OnTestSuiteEnd(const TestSuite& testSuite) override { m_printer->OnTestSuiteEnd(testSuite); }
#ifndef GTEST_REMOVE_LEGACY_TEST_CASEAPI_
		void OnTestCaseEnd(const TestCase& testCase) override { m_printer->OnTestCaseEnd(testCase); }
#endif
		void OnEnvironmentsTearDownStart(const UnitTest& unitTest) override { m_printer->OnEnvironmentsTearDownStart(unitTest); }
		void OnEnvironmentsTearDownEnd(const UnitTest& unitTest) override { m_printer->OnEnvironmentsTearDownEnd(unitTest); }
		void OnTestIterationEnd(const UnitTest& unitTest, int iteration) override { m_printer->OnTestIterationEnd(unitTest, iteration); }
		void OnTestProgramEnd(const UnitTest& unitTest) override { m_printer->OnTestProgramEnd(unitTest); }

	private:
		::testing::TestEventListener* m_printer;
	};

	bool UseColor()
	{
		const std::string& color = GTEST_FLAG(color);
		if (color == "yes") return true;
		if (color == "no") return false;
#if defined(WIN32)
		return _isatty(_fileno(stdout)) != 0;
#else
		return isatty(fileno(stdout)) != 0;
#endif
	}

	void OutputFormattedString(const char *format, ...)
	{
		va_list args;
		va_start( args, format );
		va_list sizingArgs;
		va_copy(sizingArgs, args);
		int len = vsnprintf(nullptr, 0, format, sizingArgs);
		va_end(sizingArgs);
		if (len < 0)
		{
			va_end(args);
			return;
		}
		char *str = new char[static_cast<size_t>(len) + 1];
		vsnprintf(str, len + 1, format, args );
		va_end(args);
		if (UseColor())
		{
			std::cout << "\033[1;36m" << str << "\033[0m" << std::endl;
		}
		else
		{
			std::cout << str << std::endl;
		}
		delete [] str;
	}
};

class ConsoleOutputData 
{
public:
	std::vector<std::vector<std::string> *> m_failureLines;
	std::vector<std::string> *m_failuresCurrentTestCase;

	ConsoleOutputData()
	{
	}
	~ConsoleOutputData()
	{
		for(auto iter=m_failureLines.begin();iter!=m_failureLines.end();++iter)
		{
			delete *iter;
		}
		m_failureLines.clear();
	}

	void PrintFailedTests()
	{
		for(auto iter=m_failureLines.cbegin();iter!=m_failureLines.cend();++iter)
		{
			for(auto line=(*iter)->cbegin();line != (*iter)->cend(); ++line)
			{
				OutputFormattedString((*line).c_str());
			}
		}
	}

	void StartTestCase(std::string name)
	{
		m_failuresCurrentTestCase = new std::vector<std::string>;
		m_failuresCurrentTestCase->push_back("[" + name + "]");
	}

	void AddTestCaseFailure(std::string failure)
	{
		m_failuresCurrentTestCase->push_back(failure);
	}

	void EndTestCase()
	{
		if( m_failuresCurrentTestCase->size()>1 )
		{
			m_failureLines.push_back(m_failuresCurrentTestCase);
		}
		else
		{
			delete m_failuresCurrentTestCase;
		}
	}
};

void ConsoleOutput::Start()
{
	::testing::UnitTest& unitTest = *::testing::UnitTest::GetInstance();
	::testing::TestEventListeners& listeners = unitTest.listeners();
	::testing::TestEventListener* defaultPrinter = listeners.default_result_printer();
	if(defaultPrinter)
	{
		defaultPrinter = listeners.Release(defaultPrinter);
		listeners.Append(new FilteringResultPrinter(defaultPrinter));
	}
	listeners.Append(new ConsoleOutput);
}

void ConsoleOutput::SetCompactTestOutput(bool enabled)
{
	compactTestOutput = enabled;
}

ConsoleOutput::ConsoleOutput()
{
	m_data = new ConsoleOutputData();
}

ConsoleOutput::~ConsoleOutput()
{
	delete m_data;
}

// Fired before any test activity starts.
void ConsoleOutput::OnTestProgramStart(const UnitTest&  /*unit_test*/)
{}

// Fired after all test activities have ended.
void ConsoleOutput::OnTestProgramEnd(const UnitTest&  /*unit_test*/) 
{}

// Fired after environment set-up for each iteration of tests ends.
void ConsoleOutput::OnEnvironmentsSetUpEnd(const UnitTest& /*unit_test*/)
{}

// Fired after environment tear-down for each iteration of tests ends.
void ConsoleOutput::OnEnvironmentsTearDownEnd(const UnitTest& /*unit_test*/)
{}

// Fired before the test case starts.
void ConsoleOutput::OnTestCaseStart(const TestCase& /*test_case*/) 
{}

// Fired after the test case ends.
void ConsoleOutput::OnTestCaseEnd(const TestCase& /*test_case*/) 
{}

// Called before a test starts.
void ConsoleOutput::OnTestStart(const TestInfo& /*test_info*/) 
{}

// Called after a failed assertion or a SUCCEED() invocation.
void ConsoleOutput::OnTestPartResult(const TestPartResult& testPartResult) 
{
	if( !testPartResult.failed() )
    {
		const char *message = testPartResult.message();
		if (message == nullptr) return;
		// GoogleTest prefixes the assertion text with the source location, then
		// the generic "Succeeded" text. Keep the user's streamed message.
		const char *line = strchr(message, '\n');
		if (line != nullptr) message = line + 1;
		if (strncmp(message, "Succeeded", 9) == 0 &&
			(message[9] == '\0' || message[9] == '\r' || message[9] == '\n'))
		{
			message += 9;
			if (*message == '\r') ++message;
			if (*message == '\n') ++message;
		}
		if (*message == '\0') return;

		std::vector<std::string> lines;
		const char *start = message;
		for (const char *cursor = message; ; ++cursor)
		{
			if (*cursor == '\n' || *cursor == '\0')
			{
				std::string line(start, cursor - start);
				if (!line.empty() && line.back() == '\r') line.pop_back();
				lines.push_back(line);
				if (*cursor == '\0' || cursor[1] == '\0') break;
				start = cursor + 1;
			}
		}

		for (size_t i = 0; i < lines.size(); ++i)
		{
			const char *label = "[    DEBUG ]";
			if (lines.size() > 1)
			{
				if (i == 0) label = "[ DEBUG    ]";
				else if (i + 1 < lines.size()) label = "[          ]";
			}
			OutputFormattedString("%s %s", label, lines[i].c_str());
		}
    }
}

// Called after a test ends.
void ConsoleOutput::OnTestEnd(const TestInfo& /*test_info*/) 
{}
