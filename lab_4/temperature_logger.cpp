#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <thread>
#include <algorithm>
struct TempReading {
    float temperature;
    std::time_t timestamp;
};

std::vector<TempReading> readings;

std::string getCurrentTimeStr() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

float readTemperature(const std::string& portName) {
    std::ifstream inFile(portName);
    if (!inFile) {
        std::cerr << "Failed to read from " << portName << std::endl;
        return -1.0f;
    }

    float temp;
    inFile >> temp;
    inFile.close();

//    std::ofstream clearFile(portName, std::ios::trunc);
//    clearFile.close();

    return temp;
}

void writeToLog(const std::string& filename, const std::string& message) {
    std::ofstream logFile(filename, std::ios::app);
    if (!logFile) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
        return;
    }
    logFile << message << std::endl;
    logFile.close();
}

void cleanupOldReadings() {
    auto now = std::chrono::system_clock::now();
    auto threshold = now - std::chrono::hours(24);

    readings.erase(
        std::remove_if(
            readings.begin(),
            readings.end(),
            [threshold](const TempReading& r) {
                return std::chrono::system_clock::from_time_t(r.timestamp) < threshold;
            }
        ),
        readings.end()
    );
}

float calculateHourlyAverage() {
    if (readings.empty()) return 0.0f;

    auto now = std::chrono::system_clock::now();
    auto hourAgo = now - std::chrono::hours(1);

    float sum = 0.0f;
    int count = 0;

    for (const auto& r : readings) {
        if (std::chrono::system_clock::from_time_t(r.timestamp) >= hourAgo) {
            sum += r.temperature;
            count++;
        }
    }

    return (count > 0) ? (sum / count) : 0.0f;
}

float calculateDailyAverage() {
    if (readings.empty()) return 0.0f;

    auto now = std::chrono::system_clock::now();
    auto dayAgo = now - std::chrono::hours(24);

    float sum = 0.0f;
    int count = 0;

    for (const auto& r : readings) {
        if (std::chrono::system_clock::from_time_t(r.timestamp) >= dayAgo) {
            sum += r.temperature;
            count++;
        }
    }

    return (count > 0) ? (sum / count) : 0.0f;
}

int main() {
    std::string portName = "virtual_port.txt";

    while (true) {
        float temp = readTemperature(portName);
        if (temp < 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        TempReading reading;
        reading.temperature = temp;
        reading.timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        readings.push_back(reading);

        cleanupOldReadings();
        writeToLog("all_temps.log", getCurrentTimeStr() + " | Temp: " + std::to_string(temp));

        static std::time_t lastHourlyLog = 0;
        std::time_t now = std::time(nullptr);
        if (now - lastHourlyLog >= 3600) {
            float avgHourly = calculateHourlyAverage();
            writeToLog("hourly_avg.log", getCurrentTimeStr() + " | Avg Hourly: " + std::to_string(avgHourly));
            lastHourlyLog = now;
        }

        static std::time_t lastDailyLog = 0;
        if (now - lastDailyLog >= 86400) {
            float avgDaily = calculateDailyAverage();
            writeToLog("daily_avg.log", getCurrentTimeStr() + " | Avg Daily: " + std::to_string(avgDaily));
            lastDailyLog = now;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}