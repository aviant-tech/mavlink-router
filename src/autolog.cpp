/*
 * This file is part of the MAVLink Router project
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
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
#include "autolog.h"

#include <common/log.h>

#include "binlog.h"
#include "ulog.h"

int AutoLog::write_msg(const struct buffer *buffer)
{
    if (_logger != nullptr) {
        return _logger->write_msg(buffer);
    }

    /* set the expected system id to the first autopilot that we get a heartbeat from */
    if (_target_system_id == -1 && buffer->curr.msg_id == MAVLINK_MSG_ID_HEARTBEAT
        && buffer->curr.src_compid == MAV_COMP_ID_AUTOPILOT1) {
        _target_system_id = buffer->curr.src_sysid;
    }

    if (buffer->curr.msg_id != MAVLINK_MSG_ID_HEARTBEAT
        || buffer->curr.src_sysid != _target_system_id) {
        return buffer->len;
    }

    const mavlink_heartbeat_t *heartbeat = (mavlink_heartbeat_t *)buffer->curr.payload;

    /* We check autopilot on heartbeat */
    log_debug("Got autopilot %u from heartbeat", heartbeat->autopilot);
    if (heartbeat->autopilot == MAV_AUTOPILOT_PX4) {
        _logger = std::unique_ptr<LogEndpoint>(new ULog(_config));
    } else if (heartbeat->autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) {
        _logger = std::unique_ptr<LogEndpoint>(new BinLog(_config));
    } else {
        log_warning("Unidentified autopilot, cannot start flight stack logging");
    }

    return buffer->len;
}

unsigned long AutoLog::_pre_stop()
{
    if (_logger) {
        return _logger->_pre_stop();
    }
    return 0;
}

bool AutoLog::_post_stop()
{
    if (_logger) {
        return _logger->_post_stop();
    }
    return false;
}

bool AutoLog::start()
{
    return true;
}

void AutoLog::print_statistics()
{
    if (_logger) {
        _logger->print_statistics();
    } else {
        Endpoint::print_statistics();
    }
}
