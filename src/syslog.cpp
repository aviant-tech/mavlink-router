#include "syslog.h"
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <common/log.h>

SysLogEndpoint::SysLogEndpoint(LogOptions conf)
    : LogEndpoint("SysLog", conf), _current_log_size(0)
{
    _syslog_path = "/var/log/syslog";
}

bool SysLogEndpoint::start()
{
    if (!LogEndpoint::start()) {
        return false;
    }
    return _copy_syslog();
}

bool SysLogEndpoint::_logging_start_timeout()
{
    return _copy_syslog();
}

int SysLogEndpoint::write_msg(const struct buffer *pbuf)
{
    // This method is called periodically. We can use it to check if we need to copy more logs.
    if (_current_log_size < MAX_SYSLOG_SIZE) {
        _copy_syslog();
    }
    return pbuf->len;
}

bool SysLogEndpoint::_copy_syslog()
{
    std::ifstream syslog(_syslog_path, std::ios::binary | std::ios::ate);
    if (!syslog) {
        log_error("Failed to open syslog file: %s", _syslog_path.c_str());
        return false;
    }

    std::streamsize size = syslog.tellg();
    syslog.seekg(0, std::ios::beg);

    size_t bytes_to_read = std::min(static_cast<size_t>(size), MAX_SYSLOG_SIZE - _current_log_size);
    if (bytes_to_read == 0) {
        return true; // Nothing more to copy
    }

    std::vector<char> buffer(bytes_to_read);
    if (!syslog.read(buffer.data(), bytes_to_read)) {
        log_error("Failed to read from syslog file");
        return false;
    }

    if (write(_file, buffer.data(), bytes_to_read) != static_cast<ssize_t>(bytes_to_read)) {
        log_error("Failed to write to log file");
        return false;
    }

    _current_log_size += bytes_to_read;
    return true;
}