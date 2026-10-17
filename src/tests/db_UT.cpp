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

    db.Open(SQLite::DB::memoryDB,false);
//    db.Open("/tmp/db.sql",false);

    db.ExecDML("CREATE TABLE temp(id INTEGER PRIMARY KEY ASC, key, value);");

    db.BeginTransaction();

    int key, value;

    auto statement = db.CompileStatement("INSERT INTO temp(id, key, value) VALUES(NULL, ?, ?);");

    for(key=0;key<10;++key)
    {
        value = key*3;
        statement.Bind(1,key);
        statement.Bind(2,value);
        statement.ExecQuery();
    }

    db.CommitTransaction();
}
