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
        const std::string& _host, const unsigned short& _port,
        const std::string& _username, const std::string& _password,
        const std::string& _database,
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
    Connection() :conn_(nullptr) {}
    Connection(Connection&&p) :conn_(p.conn_) { p.conn_ = nullptr; }
    Connection(MYSQL*p) :conn_(p) {}
    Connection& operator=(Connection&&p)
    {
        if (this != &p)
        {
            conn_ = p.conn_;
            p.conn_ = nullptr;
        }
        return (*this);
    }
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    ~Connection() { mysql_close(conn_); conn_ = nullptr; }
    MYSQL* operator*() { return conn_; }
private:
    MYSQL* conn_;
};

class ConnectionPool
{
public:
    ConnectionPool(const DbConfig& db_config)
        :
        config_(db_config)
    {
        current_size_ = 0;
        mysql_library_init(0, 0, 0);
        _init(config_.min_connection);
    }
    ~ConnectionPool()
    {
#ifdef __linux__
        mysql_library_end();
#endif
    }
    Connection get()
    {
        Connection conn;
        //Pool has conn
        if (pool_.try_dequeue(conn))
        {
            return conn;
        }
        else if (current_size_ < config_.max_connection)
        {
            //Allow create new conn
            conn = _make();
            ++current_size_;
            return conn;
        }
        else
        {
            //Number of conn is reach maxSize
            pool_.wait_dequeue(conn);
            return conn;
        }
    }
    void release(Connection&& _conn)
    {
        pool_.enqueue(std::move(_conn));
    }

private:
    void _init(const unsigned int& _init_size)
    {
        for (unsigned int i = 0;i < _init_size;++i)
        {
            pool_.enqueue(std::move(_make()));
            ++current_size_;
        }
    }
    Connection _make()
    {
        Connection conn(mysql_init(0));

        if (mysql_real_connect(*conn, config_.host.c_str(),
            config_.username.c_str(), config_.password.c_str(),
            config_.database.c_str(), config_.port, 0, 0))
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
    const DbConfig config_;
    std::atomic<unsigned int> current_size_;
    moodycamel::BlockingConcurrentQueue<Connection> pool_;
};
class ResultValue
{
public:
    ResultValue() :
        null_(true) {}
    ResultValue(const char* _obj) :
        data_((_obj == nullptr) ? "" : _obj),
        null_(_obj == nullptr)
    {}
    ResultValue(ResultValue&& p) :
        null_(std::move(p.null_)),
        data_(std::move(p.data_))
    {}
    ResultValue& operator=(ResultValue&& other)
    {
        if (this != &other)
        {
            null_ = std::move(other.null_);
            data_ = std::move(other.data_);
        }
        return (*this);
    }
    bool operator==(const ResultValue& _ref) { return this->data_ == _ref.data_; }
    bool operator!=(const ResultValue& _ref) { return this->data_ != _ref.data_; }
    bool null() { return null_; }
    bool empty() { return data_.empty(); }
    const std::string& to_string() { return data_; }
    int to_int32() { return std::stoi(data_); }
    unsigned int to_uint32() { return std::stoul(data_); }
    long long to_int64() { return std::stoll(data_); }
    unsigned long long to_uint64() { return std::stoull(data_); }
    float to_float() { return std::stof(data_); }
    double to_double() { return std::stod(data_); }
    bool to_bool() { return (data_.compare("1") == 0); }
private:
    std::string data_;
    bool null_;
};
typedef std::unordered_map<std::string, int> FieldType;
//Result row for result group
class ResultRow
{
public:
    typedef std::vector<ResultValue> RowType;
    ResultRow(ResultRow&&p) :
        row_(std::move(p.row_)),
        field_(p.field_)
    {
        p.field_ = nullptr;
    }
    ResultRow(RowType&& _row, FieldType* _field) :
        row_(std::move(_row)),
        field_(_field)
    {}
    ResultValue& operator[](const int& _str) { return row_.at(_str); }
    ResultValue& operator[](const char* _str) { return row_.at(field_->find(_str)->second); }
private:
    RowType row_;
    FieldType* field_;
};
}// namespace detail
//Result group for sql query
class Result
{
public:
    Result(Result&& p) :
        result_(std::move(p.result_)),
        field_(p.field_)
    {
        p.field_ = nullptr;
    }
    Result(MYSQL_RES* _mysql_result)
    {
        using namespace detail;
        //get filed name
        const unsigned int field_count = _mysql_result->field_count;
        field_ = new FieldType;
        for (unsigned int i = 0; i < field_count;++i)
        {
            field_->insert(
                std::make_pair(mysql_fetch_field(_mysql_result)->name, i));
        }
        //get row result and push it into result group
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
            result_.push_back(std::move(ResultRow(std::move(_row), field_)));
        }
        mysql_free_result(_mysql_result);
    }
    ~Result() { delete field_; }
    auto num_fields() { return field_->size(); }
    auto num_rows() { return result_.size(); }
    bool empty() { return result_.empty(); }
    auto& operator[] (const int _row) { return result_.at(_row); }
    //return result[0][_field], used for single result;
    auto& operator[] (const char* _field)
    {
        return result_.at(0)[_field];
    }
private:
    std::vector<detail::ResultRow> result_;//result group
    detail::FieldType *field_;//result group
};

//Single database class, if you only need to connect one database, use this.
class MySql
{
public:
    MySql(const DbConfig& _config) : pool_(_config) {}
    template <typename ...Args>
    void execute(Args &&... args)
    {
        //create sql sentence.
        auto str = fmt::format(std::forward<Args>(args)...);
        //get connection.
        auto conn = pool_.get();
        //execute sql
        if (mysql_query(*conn, str.c_str()))
        {
            //if failed, print sql and error info, return empty result.
            std::string _error = fmt::format("[mysqlcpp] Query error:\nSql: {}\nError: {}\n",
                str, mysql_error(*conn));
            pool_.release(std::move(conn));
            throw std::runtime_error(_error);
        }
        pool_.release(std::move(conn));
    }
    template <typename ...Args>
    Result query(Args &&... args)
    {
        //create sql sentence.
        auto str = fmt::format(std::forward<Args>(args)...);
        //get connection.
        auto conn = pool_.get();
        //execute sql
        if (mysql_query(*conn, str.c_str()))
        {
            //if failed, print sql and error info, return empty result.
            std::string _error = fmt::format("[mysqlcpp] Query error:\nSql: {}\nError: {}\n",
                str, mysql_error(*conn));
            pool_.release(std::move(conn));
            throw std::runtime_error(_error);
        }
        Result ret(mysql_use_result(*conn));
        pool_.release(std::move(conn));
        //return empty result if there is no result.
        return ret;
    }
private:
    detail::ConnectionPool pool_;
};

//Multiple database manager class
//if you need to connect one or more than one database, you can use this.
class DbManager
{
public:
    ~DbManager()
    {
        for (auto it = db_list_.begin();it != db_list_.end();)
        {
            delete it->second;
            it = db_list_.erase(it);
        }
    }
    //return false when db name is existed.
    bool insert(const char* _name, const DbConfig& _config)
    {
        if (db_list_.find(_name) != db_list_.end())
            return false;
        else
        {
            db_list_.insert(std::make_pair(_name, new zonciu::mysql::MySql(_config)));
            return true;
        }
    }
    //return false when Db not found.
    bool remove(const char* _name)
    {
        auto it = db_list_.find(_name);
        if (it != db_list_.end())
        {
            delete it->second;
            db_list_.erase(it);
            return true;
        }
        else
        {
            return false;
        }
    }
    auto get(const char* _name)
    {
#ifdef NDEBUG
        return db_list_.find(_name)->second;
#else
        auto it = db_list_.find(_name);
        if (it == db_list_.end())
            throw std::range_error(fmt::format("database {} not found.", _name));
        return it->second;
#endif
    }
    auto operator[](const char* _name)
    {
#ifdef NDEBUG
        return db_list_.find(_name)->second;
#else
        auto it = db_list_.find(_name);
        assert(it != db_list_.end());
        return it->second;
#endif
    }
private:
    std::unordered_map<std::string, zonciu::mysql::MySql*> db_list_;
};
}
}
#endif
