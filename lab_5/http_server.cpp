#include <iostream>
#include <sqlite3.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

sqlite3* db;

void init_db() {
    int rc = sqlite3_open("temps.db", &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        exit(1);
    }

    const char* sql =
        "CREATE TABLE IF NOT EXISTS raw_temps("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "temp REAL NOT NULL);"

        "CREATE TABLE IF NOT EXISTS hourly_avg("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "hour_start DATETIME NOT NULL,"
        "avg_temp REAL NOT NULL);"

        "CREATE TABLE IF NOT EXISTS daily_avg("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "day DATE NOT NULL,"
        "avg_temp REAL NOT NULL);";

    sqlite3_exec(db, sql, 0, 0, 0);
}

void log_temp(float temp) {
    char sql[256];
    sprintf(sql, "INSERT INTO raw_temps(temp) VALUES(%f);", temp);
    sqlite3_exec(db, sql, 0, 0, 0);
}

void calculate_hourly_avg() {
    const char* sql =
        "INSERT INTO hourly_avg(hour_start, avg_temp) "
        "SELECT datetime(strftime('%Y-%m-%d %H:00:00', timestamp)), AVG(temp) "
        "FROM raw_temps "
        "WHERE timestamp >= datetime('now', '-1 hour') "
        "GROUP BY strftime('%Y-%m-%d %H', timestamp);";
    sqlite3_exec(db, sql, 0, 0, 0);

    sqlite3_exec(db, "DELETE FROM raw_temps WHERE timestamp < datetime('now', '-1 day')", 0, 0, 0);
}

void calculate_daily_avg() {
    const char* sql =
        "INSERT INTO daily_avg(day, avg_temp) "
        "SELECT date(timestamp), AVG(temp) "
        "FROM raw_temps "
        "WHERE timestamp >= datetime('now', '-1 day') "
        "GROUP BY date(timestamp);";
    sqlite3_exec(db, sql, 0, 0, 0);

    sqlite3_exec(db, "DELETE FROM hourly_avg WHERE hour_start < datetime('now', '-1 month')", 0, 0, 0);
}

std::string generate_stats_response(const std::string& period) {
    sqlite3_stmt* stmt;
    std::string json = "{";
    const char* sql = nullptr;

    if (period == "current") {
        sql = "SELECT temp FROM raw_temps ORDER BY timestamp DESC LIMIT 1";
    } else if (period == "hour") {
        sql = "SELECT avg_temp FROM hourly_avg ORDER BY hour_start DESC LIMIT 1";
    } else if (period == "day") {
        sql = "SELECT avg_temp FROM daily_avg ORDER BY day DESC LIMIT 1";
    }

    if (sql && sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            json += "\"" + period + "_temp\": " + std::to_string(sqlite3_column_double(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    json += "}";
    return json;
}

void handle_request(http::request<http::string_body>& req, http::response<http::string_body>& res) {
    res.version(req.version());
    res.set(http::field::server, "Temp Server");
    res.set(http::field::content_type, "application/json");

    if (req.target() == "/current") {
        res.result(http::status::ok);
        res.body() = generate_stats_response("current");
    }
    else if (req.target() == "/hourly") {
        calculate_hourly_avg();
        res.result(http::status::ok);
        res.body() = generate_stats_response("hour");
    }
    else if (req.target() == "/daily") {
        calculate_daily_avg();
        res.result(http::status::ok);
        res.body() = generate_stats_response("day");
    }
    else {
        res.result(http::status::not_found);
        res.body() = "{\"error\": \"Not found\"}";
    }
    res.prepare_payload();
}

void run_server() {
    net::io_context ioc{1};
    tcp::acceptor acceptor{ioc, {tcp::v4(), 8080}};

    while (true) {
        tcp::socket socket{ioc};
        acceptor.accept(socket);

        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);

        http::response<http::string_body> res;
        handle_request(req, res);

        http::write(socket, res);
        socket.shutdown(tcp::socket::shutdown_send);
    }
}

int main() {
    init_db();
    std::thread server_thread(run_server);

    auto last_hourly_calc = std::chrono::system_clock::now();
    auto last_daily_calc = std::chrono::system_clock::now();

    while (true) {
		float base_temp = 22.0f;
		float daily_variation = sin(std::chrono::system_clock::now().time_since_epoch().count() / 1000000000.0f) * 5.0f;
		float random_noise = (rand() % 200 - 100) / 50.0f;

		float temp = base_temp + daily_variation + random_noise;

temp = std::max(10.0f, std::min(35.0f, temp));
        log_temp(temp);

        auto now = std::chrono::system_clock::now();
        if (now - last_hourly_calc >= std::chrono::hours(1)) {
            calculate_hourly_avg();
            last_hourly_calc = now;
        }

        if (now - last_daily_calc >= std::chrono::hours(24)) {
            calculate_daily_avg();
            last_daily_calc = now;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}