#include <stdio.h>

#include "gtest/gtest.h"

#include "ConsoleOutput.h"
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#include "VisualStudioOutput.h"

GTEST_API_ int main(int argc, char **argv) 
{
    VisualStudioOutput::Start();
#else
GTEST_API_ int main(int argc, char **argv) 
{
#endif
    ConsoleOutput::Start();
    printf("Running main() from GoogleTestMain.cpp\n");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
