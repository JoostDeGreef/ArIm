#include <stdio.h>

#include "gtest/gtest.h"

#include "ConsoleOutput.h"

GTEST_API_ int main(int argc, char **argv) 
{
    printf("Running main() from GoogleTestMain.cpp\n");
    testing::InitGoogleTest(&argc, argv);
    ConsoleOutput::Start();
    return RUN_ALL_TESTS();
}
