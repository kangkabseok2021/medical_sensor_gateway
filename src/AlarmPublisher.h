#pragma once
#include "MedicalFsm.h"
#include <string>

// Emits structured JSON alarm events to stdout and optionally to a named pipe.
// Format: {"ts":"...","event":"ALARM","trigger":"...","value":...,"unit":"..."}
class AlarmPublisher {
public:
    // pipe_path: POSIX named pipe path; empty = stdout only.
    explicit AlarmPublisher(std::string pipe_path = "");
    ~AlarmPublisher();

    void emit(MedState event, const char* trigger,
              double value, const char* unit);

private:
    std::string pipe_path_;
    int         pipe_fd_{-1};

    void write_all(const std::string& json);
};
