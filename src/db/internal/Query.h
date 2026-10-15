#pragma once

namespace SQLite
{
    class Query
    {
    public:
        class State;
    public:
        Query();
        Query(std::shared_ptr<State>&& state);
        Query(Query&& query);
        Query(const Query& query);
        ~Query();

        Query& operator = (const Query& query);

        int GetFieldCount() const;
        const std::vector<std::string>& GetFieldNames() const;
        int FieldIndex(const std::string& fieldName) const;
        const std::string& FieldName(int field) const;
        std::string FieldDeclType(int field) const;
        DataType FieldDataType(int field) const;
        std::string FieldValue(int field) const;
        std::string FieldValue(const std::string& field) const;
        int GetIntField(int field, int nullValue = 0) const;
        int GetIntField(const std::string& field, int nullValue = 0) const;
        int64_t GetInt64Field(int field, long long nullValue = 0) const;
        int64_t GetInt64Field(const std::string& field, long long nullValue = 0) const;
        double GetFloatField(int field, double nullValue = 0.0) const;
        double GetFloatField(const std::string& field, double nullValue = 0.0) const;
        std::string GetStringField(int field, const std::string& nullValue = "") const;
        std::string GetStringField(const std::string& field, const std::string& nullValue = "") const;
        std::vector<unsigned char> GetBlobField(int field) const;
        std::vector<unsigned char> GetBlobField(const std::string& field) const;
        bool FieldIsNull(int field) const;
        bool FieldIsNull(const std::string& field) const;
        bool IsEOF() const;
        void NextRow();
        void Finalize();
    private:
        std::shared_ptr<State> m_state;
    };
} // namespace SQLite

