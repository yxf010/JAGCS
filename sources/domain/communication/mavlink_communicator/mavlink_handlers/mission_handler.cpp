#include "mission_handler.h"

// MAVLink
#include <mavlink.h>

// Qt
#include <QDebug>

// Internal
#include "mavlink_communicator.h"

#include "mission_service.h"
#include "mission.h"

using namespace domain;

namespace
{
    MissionItem::Command decodeCommand(uint16_t command)
    {
        switch (command) {
        case MAV_CMD_NAV_TAKEOFF: return MissionItem::Takeoff;
        case MAV_CMD_NAV_WAYPOINT: return MissionItem::Waypoint;
        case MAV_CMD_NAV_LOITER_UNLIM:
        case MAV_CMD_NAV_LOITER_TURNS:
            return MissionItem::Loiter;
        case MAV_CMD_NAV_RETURN_TO_LAUNCH: return MissionItem::Return;
        case MAV_CMD_NAV_LAND: return MissionItem::Landing;
        default: return MissionItem::UnknownCommand;
        }
    }
}

MissionHandler::MissionHandler(MissionService* missionService,
                               MavLinkCommunicator* communicator):
    AbstractMavLinkHandler(communicator),
    m_missionService(missionService)
{
    connect(missionService, &MissionService::commandDownloadMission,
            this, &MissionHandler::requestMission);
    connect(missionService, &MissionService::commandUploadMission,
            this, &MissionHandler::sendMissionCount);
}

void MissionHandler::processMessage(const mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MISSION_COUNT:
        this->processMissionCount(message);
        break;
    case MAVLINK_MSG_ID_MISSION_ITEM:
        this->processMissionItem(message);
        break;
    case MAVLINK_MSG_ID_MISSION_REQUEST:
        this->processMissionRequest(message);
        break;
    case MAVLINK_MSG_ID_MISSION_ACK:
        this->processMissionAct(message);
        break;
    default:
        break;
    }
}

void MissionHandler::requestMission(uint8_t id)
{
    mavlink_message_t message;
    mavlink_mission_request_list_t request;

    // TODO: request Timer

    request.target_system = id;
    request.target_component = MAV_COMP_ID_MISSIONPLANNER;

    mavlink_msg_mission_request_list_encode(m_communicator->systemId(),
                                            m_communicator->componentId(),
                                            &message, &request);
    m_communicator->sendMessageAllLinks(message);
}

void MissionHandler::requestMissionItem(uint8_t id, uint16_t seq)
{
    mavlink_message_t message;
    mavlink_mission_request_t missionRequest;

    missionRequest.target_system = id;
    missionRequest.target_component = MAV_COMP_ID_MISSIONPLANNER;
    missionRequest.seq = seq;

    mavlink_msg_mission_request_encode(m_communicator->systemId(),
                                       m_communicator->componentId(),
                                       &message, &missionRequest);
    m_communicator->sendMessageAllLinks(message);
}

void MissionHandler::sendMissionCount(uint8_t id)
{
    mavlink_message_t message;
    mavlink_mission_count_t count;

    count.target_system = id;
    count.target_component = MAV_COMP_ID_MISSIONPLANNER;
    count.count = m_missionService->requestMissionForVehicle(id)->count();

    mavlink_msg_mission_count_encode(m_communicator->systemId(),
                                     m_communicator->componentId(),
                                     &message, &count);
    m_communicator->sendMessageAllLinks(message);
}

void MissionHandler::sendMissionItem(uint8_t id, uint16_t seq)
{
    Mission* mission = m_missionService->requestMissionForVehicle(id);
    if (mission->count() <= seq) return;

    MissionItem* item = mission->item(seq);

    mavlink_message_t message;
    mavlink_mission_item_t msgItem;

    msgItem.target_system = id;
    msgItem.target_component = MAV_COMP_ID_MISSIONPLANNER;
    // TODO: encode Mission Item

    mavlink_msg_mission_item_encode(m_communicator->systemId(),
                                    m_communicator->componentId(),
                                    &message, &msgItem);
    m_communicator->sendMessageAllLinks(message);
}

void MissionHandler::processMissionCount(const mavlink_message_t& message)
{
    Mission* mission = m_missionService->requestMissionForVehicle(message.sysid);

    mavlink_mission_count_t missionCount;
    mavlink_msg_mission_count_decode(&message, &missionCount);

    mission->setCount(missionCount.count);

    for (uint16_t seq = 0; seq < missionCount.count; ++seq)
    {
        this->requestMissionItem(message.sysid, seq);
    }
}

void MissionHandler::processMissionItem(const mavlink_message_t& message)
{
    Mission* mission = m_missionService->requestMissionForVehicle(message.sysid);

    mavlink_mission_item_t msgItem;
    mavlink_msg_mission_item_decode(&message, &msgItem);

    MissionItem* item = mission->requestItem(msgItem.seq);

    switch (msgItem.frame)
    {
    case MAV_FRAME_GLOBAL:
        item->setGlobalCoordinate(msgItem.x, msgItem.y, msgItem.z);
        break;
    case MAV_FRAME_GLOBAL_RELATIVE_ALT:
        item->setCoordinateRelativeAltitude(msgItem.x, msgItem.y, msgItem.z);
        break;
    default:
        break;
    }

    item->setCommand(::decodeCommand(msgItem.command));
    item->setCurrent(msgItem.current);
}

void MissionHandler::processMissionRequest(const mavlink_message_t& message)
{
    mavlink_mission_request_t request;
    mavlink_msg_mission_request_decode(&message, &request);

    this->sendMissionItem(message.sysid, request.seq);
}

void MissionHandler::processMissionAct(const mavlink_message_t& message)
{
    Mission* mission = m_missionService->requestMissionForVehicle(message.sysid);

    mavlink_mission_ack_t missionAck;
    mavlink_msg_mission_ack_decode(&message, &missionAck);

    // TODO: handle missionAck
}