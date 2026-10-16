#include "db.h"

#include <gtest/gtest.h>
using namespace testing;

class SQLiteTest : public Test 
{
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown() 
    {
    }

};

TEST_F(SQLiteTest, SmokeTest)
{
    SQLite::DB db;

    db.Open("/tmp/smoketest.db",false);
}
