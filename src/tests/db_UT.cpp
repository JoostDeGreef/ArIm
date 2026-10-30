#include <utility>

#include "googletest_wrapper.h"
#include "db.h"
#include "SQLiteException.h"

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

    int res = db.ExecDML("CREATE TABLE temp(id INTEGER PRIMARY KEY ASC, key, value);");
    ASSERT_EQ(0,res);
    ASSERT_TRUE(db.TableExists("temp"));

    {
        db.BeginTransaction();
        auto statement = db.CompileStatement("INSERT INTO temp(id, key, value) VALUES(NULL, ?, ?);");
        for(int key=0;key<10;++key)
        {
            int value = key*3;
            statement.Bind(1,key);
            statement.Bind(2,value);
            statement.ExecDML();
        }
        db.CommitTransaction();
    }

    {
        auto statement = db.CompileStatement("SELECT id, key, value FROM temp;");
        auto query = statement.ExecQuery();
        auto names = query.GetFieldNames();
        ASSERT_EQ(3,names.size());
        EXPECT_EQ("id", names[0]);
        EXPECT_EQ("key", names[1]);
        EXPECT_EQ("value", names[2]);
        int i = 1;
        for(auto & row : query.GetRows())
        {
            EXPECT_EQ(i, row.GetIntField(0));
            i++;
        }
    }

}

TEST_F(SQLiteTest, OwnershipTest_DestroyDbWithActiveStatement)
{
    SQLite::Statement statement;
    {
        SQLite::DB db;
        db.Open(SQLite::DB::memoryDB, false);
        statement = db.CompileStatement("SELECT 1;");
    }

    // close_v2 defers closing the connection until the statement is finalized.
    statement.Finalize();
}

TEST_F(SQLiteTest, OwnershipTest_FinalizeStatementTwice)
{
    SQLite::DB db;
    db.Open(SQLite::DB::memoryDB, false);
    auto statement = db.CompileStatement("SELECT 1;");

    EXPECT_NO_THROW(statement.Finalize());
    EXPECT_NO_THROW(statement.Finalize());
    EXPECT_THROW(statement.Bind(1, 1), SQLite::Exception);
}

TEST_F(SQLiteTest, OwnershipTest_CloseDbWithActiveStatement)
{
    SQLite::DB db;
    db.Open(SQLite::DB::memoryDB, false);
    auto statement = db.CompileStatement("SELECT 1;");

    EXPECT_NO_THROW(db.Close());
    EXPECT_NO_THROW(statement.Finalize());
}

TEST_F(SQLiteTest, OwnershipTest_MoveStatement)
{
    SQLite::DB db;
    db.Open(SQLite::DB::memoryDB, false);
    db.ExecDML("CREATE TABLE values_table(value INTEGER);");

    auto source = db.CompileStatement("INSERT INTO values_table VALUES(?);");
    SQLite::Statement moved(std::move(source));
    EXPECT_NO_THROW(source.Finalize());
    moved.Bind(1, 42);
    EXPECT_EQ(1, moved.ExecDML());

    auto destination = db.CompileStatement("SELECT 1;");
    auto assignmentSource = db.CompileStatement("INSERT INTO values_table VALUES(?);");
    destination = std::move(assignmentSource);
    EXPECT_NO_THROW(assignmentSource.Finalize());
    destination.Bind(1, 43);
    EXPECT_EQ(1, destination.ExecDML());
    EXPECT_EQ(2, db.ExecSingleInt64("SELECT COUNT(*) FROM values_table;"));
}

TEST_F(SQLiteTest, OwnershipTest_QueryRowsFinalizeOnEarlyExit)
{
    SQLite::DB db;
    db.Open(SQLite::DB::memoryDB, false);

    auto query = db.ExecQuery("SELECT 1 UNION ALL SELECT 2;");
    {
        auto rows = query.GetRows();
        for (auto& row : rows)
        {
            EXPECT_EQ(1, row.GetIntField(0));
            break;
        }
    }

    EXPECT_EQ(7, db.ExecSingleInt64("SELECT 7;"));
}

TEST_F(SQLiteTest, OwnershipTest_MoveQuery)
{
    SQLite::DB db;
    db.Open(SQLite::DB::memoryDB, false);

    auto source = db.ExecQuery("SELECT 42;");
    SQLite::Query moved(std::move(source));
    EXPECT_NO_THROW(source.Finalize());
    EXPECT_EQ(42, moved.GetIntField(0));

    auto assignmentSource = db.ExecQuery("SELECT 43;");
    auto destination = db.ExecQuery("SELECT 0;");
    destination = std::move(assignmentSource);
    EXPECT_NO_THROW(assignmentSource.Finalize());
    EXPECT_EQ(43, destination.GetIntField(0));
}
