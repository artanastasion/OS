#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#endif

bool openSerialPort(const std::string& portName) {
#ifdef _WIN32
    HANDLE hSerial = CreateFileA(portName.c_str(), GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open port " << portName << std::endl;
        return false;
    }
    CloseHandle(hSerial);
#else
    int fd = open(portName.c_str(), O_WRONLY);
    if (fd == -1) {
        std::cerr << "Failed to open port " << portName << std::endl;
        return false;
    }
    close(fd);
#endif
    return true;
}

void writeTemperature(const std::string& portName, float temp) {
    std::ofstream outFile(portName, std::ios::app);
    if (!outFile) {
        std::cerr << "Failed to write to " << portName << std::endl;
        return;
    }
    outFile << temp << std::endl;
    outFile.close();
}

int main() {
    srand(time(0));
    std::string portName = "virtual_port.txt";

    if (!openSerialPort(portName)) {
        return 1;
    }

    while (true) {
        float temp = 15.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 15.0f));

        writeTemperature(portName, temp);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}