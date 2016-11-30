#ifndef ZONCIU_MYSQL_H
#define ZONCIU_MYSQL_H

#include "3rd/concurrentqueue/blockingconcurrentqueue.h"
#include "zonciu/format.hpp"
#include "mysql.h"
#include <assert.h>
#include <array>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>
namespace zonciu
{
namespace mysql
{
struct DbConfig
{
    unsigned short port;
    unsigned int min_connection;
    unsigned int max_connection;
    std::string host;
    std::string username;
    std::string password;
    std::string database;
    DbConfig() {}
    DbConfig(
        const std::string _host, const unsigned short _port,
        const std::string _username, const std::string _password,
        const std::string _database,
        const unsigned int _min_size, const unsigned int _max_size)
        :
        host(_host), port(_port), username(_username), password(_password),
        database(_database), min_connection(_min_size), max_connection(_max_size)
    {}
};

namespace detail
{
class Connection
{
public:
    Connection() :_conn(nullptr) {}
    Connection(Connection&&p) :_conn(p._conn) { p._conn = nullptr; }
    Connection(MYSQL*p) :_conn(p) {}
    Connection& operator=(Connection&&p)
    {
        if (this != &p)
        {
            _conn = p._conn;
            p._conn = nullptr;
        }
        return (*this);
    }
    ~Connection() { mysql_close(_conn); _conn = nullptr; }
    MYSQL* operator*() { return _conn; }
private:
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    MYSQL* _conn;
};

class ConnectionPool
{
public:
    ConnectionPool(const DbConfig& db_config)
        :
        _config(db_config)
    {
        _current_size = 0;
        mysql_library_init(0, 0, 0);
        _Init(_config.min_connection);
    }
    ~ConnectionPool()
    {
#ifdef __linux__
        mysql_library_end();
#endif
    }
    Connection Get()
    {
        Connection conn;
        //Pool has conn
        if (_pool.try_dequeue(conn))
        {
            return conn;
        }
        else if (_current_size < _config.max_connection)
        {
            //Allow create new conn
            conn = _Connect();
            ++_current_size;
            return conn;
        }
        else
        {
            //Number of conn is reach maxSize
            _pool.wait_dequeue(conn);
            return conn;
        }
    }
    void Release(Connection&& _conn)
    {
        _pool.enqueue(std::move(_conn));
    }

private:
    void _Init(const unsigned int& _init_size)
    {
        for (unsigned int i = 0;i < _init_size;++i)
        {
            _pool.enqueue(std::move(_Connect()));
            ++_current_size;
        }
    }
    Connection _Connect()
    {
        Connection conn(mysql_init(0));

        if (mysql_real_connect(*conn, _config.host.c_str(),
            _config.username.c_str(), _config.password.c_str(),
            _config.database.c_str(), _config.port, 0, 0))
        {
            my_bool reconnect = 1;
            mysql_options(*conn, MYSQL_OPT_RECONNECT, &reconnect);
            return conn;
        }
        else
        {
            throw std::runtime_error(
                fmt::format("[mysqlcpp] Connect failed: {}",
                mysql_error(*conn)));
        }
    }
    const DbConfig _config;
    std::atomic<unsigned int> _current_size;
    moodycamel::BlockingConcurrentQueue<Connection> _pool;
};
class ResultValue
{
public:
    ResultValue() :
        _null(true) {}
    ResultValue(const char* _obj) :
        _data((_obj == nullptr) ? "" : _obj),
        _null(_obj == nullptr)
    {}
    ResultValue(ResultValue&& p) :
        _null(std::move(p._null)),
        _data(std::move(p._data))
    {}
    ResultValue& operator=(ResultValue&& other)
    {
        if (this != &other)
        {
            _null = std::move(other._null);
            _data = std::move(other._data);
        }
        return (*this);
    }
    bool operator==(const ResultValue& _ref) { return this->_data == _ref._data; }
    bool operator!=(const ResultValue& _ref) { return this->_data != _ref._data; }
    bool IsNull() { return _null; }
    bool IsEmpty() { return _data.empty(); }
    const std::string& GetString() { return _data; }
    int GetInt32() { return std::stoi(_data); }
    unsigned int GetUint32() { return std::stoul(_data); }
    long long GetInt64() { return std::stoll(_data); }
    unsigned long long GetUint64() { return std::stoull(_data); }
    float GetFloat() { return std::stof(_data); }
    double GetDouble() { return std::stod(_data); }
    bool GetBool() { return (_data.compare("1") == 0); }
private:
    std::string _data;
    bool _null;
};
typedef std::unordered_map<std::string, int> FieldType;
//Result row for result group
class ResultRow
{
public:
    typedef std::vector<ResultValue> RowType;
    ResultRow(ResultRow&&p) :
        _row(std::move(p._row)),
        _field(p._field)
    {
        p._field = nullptr;
    }
    ResultRow(RowType&& _row, FieldType* _field) :
        _row(std::move(_row)),
        _field(_field)
    {}
    ResultValue& operator[](const int& _str) { return _row.at(_str); }
    ResultValue& operator[](const char* _str) { return _row.at(_field->find(_str)->second); }
private:
    RowType _row;
    FieldType* _field;
};
}// namespace detail
//Result group for sql Query
class Result
{
public:
    Result(Result&& p) :
        _result(std::move(p._result)),
        _field(p._field)
    {
        p._field = nullptr;
    }
    Result(MYSQL_RES* _mysql_result)
    {
        using namespace detail;
        //Get filed name
        const unsigned int field_count = _mysql_result->field_count;
        _field = new FieldType;
        for (unsigned int i = 0; i < field_count;++i)
        {
            _field->insert(
                std::make_pair(mysql_fetch_field(_mysql_result)->name, i));
        }
        //Get row result and push it into result group
        ResultRow::RowType _row;
        for (MYSQL_ROW _mysql_row = mysql_fetch_row(_mysql_result);
            _mysql_row != nullptr;
            _mysql_row = mysql_fetch_row(_mysql_result))
        {
            _row.resize(field_count);
            for (int i = field_count - 1; i > -1;--i)
            {
                _row[i] = std::move(ResultValue(_mysql_row[i]));
            }
            _result.push_back(std::move(ResultRow(std::move(_row), _field)));
        }
        mysql_free_result(_mysql_result);
    }
    ~Result() { delete _field; }
    size_t NumField() { return _field->size(); }
    size_t NumRow() { return _result.size(); }
    bool IsEmpty() { return _result.empty(); }
    detail::ResultRow& operator[] (const int _row) { return _result.at(_row); }
    //return result[0][_field], used for single result;
    detail::ResultValue& operator[] (const char* _field)
    {
        return _result.at(0)[_field];
    }
private:
    std::vector<detail::ResultRow> _result;//result group
    detail::FieldType *_field;//result group
};

//Single database class, if you only need to connect one database, use this.
class MySql
{
public:
    MySql(const DbConfig& _config) : _pool(_config) {}
    template <typename ...Args>
    void Execute(Args &&... args)
    {
        //create sql sentence.
        auto str = fmt::format(std::forward<Args>(args)...);
        //Get connection.
        auto conn = _pool.Get();
        //Execute sql
        if (mysql_query(*conn, str.c_str()))
        {
            //if failed, print sql and error info, return empty result.
            std::string _error = fmt::format("[mysqlcpp] Query error:\nSql: {}\nError: {}\n",
                str, mysql_error(*conn));
            _pool.Release(std::move(conn));
            throw std::runtime_error(_error);
        }
        _pool.Release(std::move(conn));
    }
    template <typename ...Args>
    Result Query(Args &&... args)
    {
        //create sql sentence.
        auto str = fmt::format(std::forward<Args>(args)...);
        //Get connection.
        auto conn = _pool.Get();
        //Execute sql
        if (mysql_query(*conn, str.c_str()))
        {
            //if failed, print sql and error info, return empty result.
            std::string _error = fmt::format("[mysqlcpp] Query error:\nSql: {}\nError: {}\n",
                str, mysql_error(*conn));
            _pool.Release(std::move(conn));
            throw std::runtime_error(_error);
        }
        Result ret(mysql_use_result(*conn));
        _pool.Release(std::move(conn));
        //return empty result if there is no result.
        return ret;
    }
private:
    detail::ConnectionPool _pool;
};

//Multiple database manager class
//if you need to connect one or more than one database, you can use this.
class DbManager
{
public:
    ~DbManager()
    {
        for (auto it = _db_list.begin();it != _db_list.end();)
        {
            delete it->second;
            it = _db_list.erase(it);
        }
    }
    //return false when db name is existed.
    bool Insert(const char* _name, const DbConfig& _config)
    {
        if (_db_list.find(_name) != _db_list.end())
            return false;
        else
        {
            _db_list.insert(std::make_pair(_name, new zonciu::mysql::MySql(_config)));
            return true;
        }
    }
    //return false when Db not found.
    bool Remove(const char* _name)
    {
        auto it = _db_list.find(_name);
        if (it != _db_list.end())
        {
            delete it->second;
            _db_list.erase(it);
            return true;
        }
        else
        {
            return false;
        }
    }
    auto Get(const char* _name)
    {
        auto it = _db_list.find(_name);
        assert(it != _db_list.end());
        return it->second;
    }
    auto operator[](const char* _name)
    {
        auto it = _db_list.find(_name);
        assert(it != _db_list.end());
        return it->second;
    }
private:
    std::unordered_map<std::string, zonciu::mysql::MySql*> _db_list;
};
}
}
#endif
