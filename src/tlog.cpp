/*
 * This file is part of the MAVLink Router project
 *
 * Copyright (C) 2023 Petrosilius <petrosilius@searchwing.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "tlog.h"

#include <assert.h>
#include <endian.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>

#include <common/log.h>
#include <common/util.h>

bool TLog::_logging_start_timeout()
{
    //_target_system_id = -1;
    return true;
}

bool TLog::start()
{
    return LogEndpoint::start();
}

int TLog::write_msg(const struct buffer *buffer)
{
    /* set the expected system id to the first autopilot that we get a heartbeat from */
    if (_target_system_id == -1 && buffer->curr.msg_id == MAVLINK_MSG_ID_HEARTBEAT
        && buffer->curr.src_compid == MAV_COMP_ID_AUTOPILOT1) {
        _target_system_id = buffer->curr.src_sysid;
    }

    /* Check if we should start or stop logging */
    _handle_auto_start_stop(buffer);

    /* Check if logging is enabled and file is open */
    if (_file == -1) {
        return buffer->len;
    }

    if (_config.telemetry_ignore_logging_data) {
        // Silently ignore LOGGING_DATA MAVLink messages here, if configured.
        if (buffer->curr.msg_id == MAVLINK_MSG_ID_LOGGING_DATA) return buffer->len;
        if (buffer->curr.msg_id == MAVLINK_MSG_ID_LOGGING_DATA_ACKED) return buffer->len;
        if (buffer->curr.msg_id == MAVLINK_MSG_ID_LOGGING_ACK) return buffer->len;
    }

    uint64_t ms_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(
                                  std::chrono::system_clock::now().time_since_epoch())
                                  .count();

    ms_since_epoch = htobe64(ms_since_epoch);

    write(_file, (void *)&ms_since_epoch, sizeof(uint64_t));
    write(_file, buffer->data, buffer->len);
    return buffer->len;
}

LogMode TLog::_get_log_mode() const {
    if (_config.telemetry_log_mode == LogMode::default_mode) {
        return _config.log_mode;
    } else {
        return _config.telemetry_log_mode;
    }
}
