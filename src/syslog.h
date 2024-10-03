#pragma once

#include "logendpoint.h"

class SysLogEndpoint : public LogEndpoint {
public:
    SysLogEndpoint(LogOptions conf);
    bool start() override;
    int write_msg(const struct buffer *pbuf) override;
    int flush_pending_msgs() override { return -ENOSYS; }

protected:
    ssize_t _read_msg(uint8_t *buf, size_t len) override { return 0; };
    bool _logging_start_timeout() override;
    const char *_get_logfile_extension() override { return "syslog"; };

private:
    static constexpr size_t MAX_SYSLOG_SIZE = 10 * 1024 * 1024; // 10 MB
    std::string _syslog_path;
    size_t _current_log_size;

    bool _copy_syslog();
};