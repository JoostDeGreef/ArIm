#include <sqlite3.h>

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
    sqlite3 *db;
	
    sqlite3_open("/tmp/smoketest.db", &db);
    ASSERT_NE(nullptr, db);

    sqlite3_close(db);
}

