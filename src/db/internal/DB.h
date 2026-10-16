#pragma once

namespace SQLite
{
  class DB
  {
  public:
    class State;
  public:
    DB();
    ~DB();

    void Open(const std::string& fileName, bool readOnly);
    void Close();
    bool TableExists(const std::string& tableName);

    // returns the number of changed rows
    int ExecDML(const std::string& sql);
    template<typename T0, typename... TN>
    int ExecDML(const std::string& fmt, T0&& t0, TN&&... tn)
    {
      return ExecDML(FormatArgs(fmt, t0, tn...));
    }

    int64_t ExecSingleInt64(const std::string& sql);
    template<typename T0, typename... TN>
    int64_t ExecSingleInt64(const std::string& fmt, T0&& t0, TN&&... tn)
    {
      return ExecSingleInt64(FormatArgs(fmt, t0, tn...));
    }

    std::string ExecSingleString(const std::string& sql);
    template<typename T0, typename... TN>
    std::string ExecSingleString(const std::string& fmt, T0&& t0, TN&&... tn)
    {
      return ExecSingleString(FormatArgs(fmt, t0, tn...));
    }

    double ExecSingleFloat(const std::string& sql);
    template<typename T0, typename... TN>
    double ExecSingleFloat(const std::string& fmt, T0&& t0, TN&&... tn)
    {
      return ExecSingleFloat(FormatArgs(fmt, t0, tn...));
    }

    Query ExecQuery(const std::string& sql);
    template<typename T0, typename... TN>
    Query ExecQuery(const std::string& fmt, T0&& t0, TN&&... tn)
    {
      return ExecQuery(FormatArgs(fmt, t0, tn...));
    }

    Statement CompileStatement(const std::string& sql);
    template<typename T0, typename... TN>
    Statement CompileStatement(const std::string& fmt, T0&& t0, TN&&... tn)
    {
      return CompileStatement(FormatArgs(fmt, t0, tn...));
    }

    void BeginTransaction();
    void CommitTransaction();

    int64_t LastRowId() const;
    void Interrupt();
    void SetBusyTimeout(int milliSecs);
    static std::string SQLiteVersion();
  private:
    template<typename... Arguments>
    std::string FormatArgs(const std::string & fmt, const Arguments&... args)
    {
        return std::format(fmt, args...);
    }

    std::shared_ptr<State> m_state;
  };
} // namespace SQLite

