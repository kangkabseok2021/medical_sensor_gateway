#include "AlarmPublisher.h"
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>

static std::string iso8601_now() {
    using namespace std::chrono;
    auto now  = system_clock::now();
    auto tt   = system_clock::to_time_t(now);
    std::tm  tm{};
    gmtime_r(&tt, &tm);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

AlarmPublisher::AlarmPublisher(std::string pipe_path)
    : pipe_path_(std::move(pipe_path)) {
    if (!pipe_path_.empty()) {
        // Open non-blocking; if reader not yet attached, writes are silently
        // dropped until a reader connects — acceptable for alarm pipe.
        pipe_fd_ = ::open(pipe_path_.c_str(), O_WRONLY | O_NONBLOCK);
    }
}

AlarmPublisher::~AlarmPublisher() {
    if (pipe_fd_ >= 0)
        ::close(pipe_fd_);
}

void AlarmPublisher::write_all(const std::string& json) {
    std::cout << json << '\n';
    std::cout.flush();
    if (pipe_fd_ >= 0) {
        std::string line = json + '\n';
        ::write(pipe_fd_, line.c_str(), line.size());
    }
}

void AlarmPublisher::emit(MedState event, const char* trigger,
                          double value, const char* unit) {
    std::ostringstream ss;
    ss << "{"
       << "\"ts\":\"" << iso8601_now() << "\","
       << "\"event\":\"" << med_state_name(event) << "\","
       << "\"trigger\":\"" << trigger << "\","
       << "\"value\":" << std::fixed << std::setprecision(3) << value << ","
       << "\"unit\":\"" << unit << "\""
       << "}";
    write_all(ss.str());
}
