#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <libpq-fe.h>

class PGConnection {
public:
    PGConnection(const std::string& conn_str) {
        conn_ = PQconnectdb(conn_str.c_str());
        if (PQstatus(conn_) != CONNECTION_OK) {
            std::string err = PQerrorMessage(conn_);
            PQfinish(conn_);
            throw std::runtime_error("Connection to database failed: " + err);
        }
        std::cout << "[DB] Connected to PostgreSQL successfully." << std::endl;
    }

    ~PGConnection() {
        if (conn_) PQfinish(conn_);
    }

    std::string ExecuteScalar(const std::string& sql) {
        PGresult* res = PQexec(conn_, sql.c_str());

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            std::string err = PQerrorMessage(conn_);
            PQclear(res);
            throw std::runtime_error("Query failed: " + err);
        }

        std::string result_str;
        if (PQntuples(res) > 0 && PQnfields(res) > 0) {
            char* val = PQgetvalue(res, 0, 0);
            if (val) result_str = std::string(val);
        }

        PQclear(res);
        return result_str;
    }

    void ExecuteCommand(const std::string& sql) {
        PGresult* res = PQexec(conn_, sql.c_str());
        if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
            std::string err = PQerrorMessage(conn_);
            PQclear(res);
            throw std::runtime_error("Command failed: " + err);
        }
        PQclear(res);
    }

private:
    PGconn* conn_;
};