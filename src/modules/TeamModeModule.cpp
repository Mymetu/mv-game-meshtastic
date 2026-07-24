#include "TeamModeModule.h"
#include "buzz/buzz.h"
#include "configuration.h"
#include "mesh/MeshService.h"
#include "mesh/MeshTypes.h"
#include "mesh/RadioLibInterface.h"
#include <esp_random.h>

// Buzzer: ToneDuration / playTones are defined in buzz.cpp but not exposed in buzz.h,
// so we forward-declare them here together with the NOTE_* constants.
struct ToneDuration {
    int frequency_khz;
    int duration_ms;
};
extern void playTones(const ToneDuration *tone_durations, int size);

#define NOTE_SILENT 1
#define NOTE_B3     247

// Private PortNum (258 = unused in 256-511 range, confirmed safe)
#define TEAM_MODE_PORTNUM static_cast<meshtastic_PortNum>(258)

// Time constants
#define BEACON_CHECK_INTERVAL_MS 5000    // Check every 5s
#define MEMBER_TIMEOUT_MS 120000         // 2 minutes = lost
#define DISCOVERED_TEAM_STALE_MS 60000   // 60s = stale entry removed

TeamModeModule *teamModeModule = nullptr;

// Static member init
TeamState TeamModeModule::state = TEAM_UNJOINED;
uint32_t TeamModeModule::teamId = 0;
uint32_t TeamModeModule::leaderNodeNum = 0;
uint32_t TeamModeModule::lastBeaconTimeMs = 0;
uint32_t TeamModeModule::lastBroadcastTimeMs = 0;
bool TeamModeModule::disconnected = false;
bool TeamModeModule::alertSilenced = false;
uint32_t TeamModeModule::broadcastIntervalMs = 15000;
std::vector<DiscoveredTeam> TeamModeModule::discoveredTeams;

TeamModeModule::TeamModeModule()
    : concurrency::OSThread("TeamMode")
{
    setInterval(BEACON_CHECK_INTERVAL_MS);
    LOG_INFO("TeamMode: module initialized, State=UNJOINED");
}

uint32_t TeamModeModule::getLastBeaconAge() const
{
    if (lastBeaconTimeMs == 0)
        return UINT32_MAX;
    return millis() - lastBeaconTimeMs;
}

void TeamModeModule::createTeam()
{
    // Generate random team ID
    teamId = esp_random();
    leaderNodeNum = nodeDB->getNodeNum();
    state = TEAM_LEADER;
    disconnected = false;
    alertSilenced = false;
    lastBroadcastTimeMs = 0; // broadcast immediately on next runOnce
    discoveredTeams.clear();

    LOG_INFO("TeamMode: created team 0x%08X, NodeNum=%u", teamId, leaderNodeNum);
}

void TeamModeModule::joinTeam(uint32_t tId, uint32_t leaderNode)
{
    teamId = tId;
    leaderNodeNum = leaderNode;
    state = TEAM_MEMBER;
    lastBeaconTimeMs = millis(); // start countdown from now
    disconnected = false;
    alertSilenced = false;
    discoveredTeams.clear();

    LOG_INFO("TeamMode: joined team 0x%08X, leader=%u", teamId, leaderNodeNum);

    // Send join request to leader
    sendJoinRequest(teamId, leaderNodeNum);
}

void TeamModeModule::leaveTeam()
{
    state = TEAM_UNJOINED;
    teamId = 0;
    leaderNodeNum = 0;
    disconnected = false;
    alertSilenced = false;
    lastBeaconTimeMs = 0;
    lastBroadcastTimeMs = 0;
    discoveredTeams.clear();

    LOG_INFO("TeamMode: left team, State=UNJOINED");
}

void TeamModeModule::silenceAlert()
{
    alertSilenced = true;
    LOG_INFO("TeamMode: alert silenced");
}

void TeamModeModule::broadcastBeacon()
{
    meshtastic_MeshPacket *p = packetPool.allocZeroed();
    p->to = NODENUM_BROADCAST;
    p->hop_limit = 0; // One-hop only, no forwarding
    p->decoded.portnum = TEAM_MODE_PORTNUM;
    p->decoded.payload.size = 9;

    // Payload: teamId(4) | leaderNodeNum(4) | action(1) = 'B'
    uint8_t payload[9];
    memcpy(payload, &teamId, 4);
    memcpy(payload + 4, &leaderNodeNum, 4);
    payload[8] = 'B'; // Beacon
    memcpy(p->decoded.payload.bytes, payload, 9);

    service->sendToMesh(p, RX_SRC_LOCAL);
    lastBroadcastTimeMs = millis();
    LOG_DEBUG("TeamMode: beacon sent, teamId=0x%08X", teamId);
}

void TeamModeModule::sendJoinRequest(uint32_t tId, uint32_t leaderNode)
{
    meshtastic_MeshPacket *p = packetPool.allocZeroed();
    p->to = leaderNode;
    p->decoded.portnum = TEAM_MODE_PORTNUM;
    p->decoded.payload.size = 9;

    uint32_t myNodeNum = nodeDB->getNodeNum();
    uint8_t payload[9];
    memcpy(payload, &tId, 4);
    memcpy(payload + 4, &myNodeNum, 4);
    payload[8] = 'J'; // Join request
    memcpy(p->decoded.payload.bytes, payload, 9);

    service->sendToMesh(p, RX_SRC_LOCAL);
    LOG_INFO("TeamMode: join request sent to node %u", leaderNode);
}

void TeamModeModule::handleTeamPacket(const meshtastic_MeshPacket *p)
{
    if (p->decoded.payload.size < 9)
        return;

    uint8_t action = p->decoded.payload.bytes[8];
    uint32_t rxTeamId;
    uint32_t rxNodeNum;
    memcpy(&rxTeamId, p->decoded.payload.bytes, 4);
    memcpy(&rxNodeNum, p->decoded.payload.bytes + 4, 4);

    switch (action) {
    case 'B': // Beacon from leader
        // If we're scanning (UNJOINED), add to discovered list
        if (state == TEAM_UNJOINED) {
            // Update existing or add new
            bool found = false;
            for (auto &t : discoveredTeams) {
                if (t.teamId == rxTeamId) {
                    t.lastSeenMs = millis();
                    t.leaderNodeNum = rxNodeNum;
                    found = true;
                    break;
                }
            }
            if (!found) {
                DiscoveredTeam dt;
                dt.teamId = rxTeamId;
                dt.leaderNodeNum = rxNodeNum;
                dt.lastSeenMs = millis();
                discoveredTeams.push_back(dt);
                LOG_INFO("TeamMode: discovered team 0x%08X from node %u", rxTeamId, rxNodeNum);
            }
        }

        // If we're a member of this team, reset lastSeen
        if (state == TEAM_MEMBER && rxTeamId == teamId) {
            lastBeaconTimeMs = millis();
            if (disconnected) {
                disconnected = false;
                alertSilenced = false;
                LOG_INFO("TeamMode: reconnected to team!");
            }
        }
        break;

    case 'J': // Join request from member
        if (state == TEAM_LEADER && rxTeamId == teamId) {
            LOG_INFO("TeamMode: member %u joined team 0x%08X", rxNodeNum, rxTeamId);
            // Could maintain a member list here if desired
        }
        break;
    }
}

void TeamModeModule::playDisconnectedTone()
{
    // Two short beeps: B3 beep, pause, B3 beep
    ToneDuration melody[] = {
        {NOTE_B3, 80},       // beep 1
        {NOTE_SILENT, 100},   // silent pause
        {NOTE_B3, 80},       // beep 2
    };
    playTones(melody, sizeof(melody) / sizeof(ToneDuration));
}

int32_t TeamModeModule::runOnce()
{
    switch (state) {
    case TEAM_LEADER:
        // Broadcast beacon at configured interval
        if (millis() - lastBroadcastTimeMs >= broadcastIntervalMs) {
            broadcastBeacon();
        }
        return BEACON_CHECK_INTERVAL_MS;

    case TEAM_MEMBER:
        // Check if we've lost the leader signal
        if (lastBeaconTimeMs > 0 && (millis() - lastBeaconTimeMs) > MEMBER_TIMEOUT_MS) {
            if (!disconnected) {
                disconnected = true;
                alertSilenced = false;
                LOG_WARN("TeamMode: lost contact with team!");
            }
            // Play alert if not silenced
            if (!alertSilenced) {
                playDisconnectedTone();
            }
        }
        return BEACON_CHECK_INTERVAL_MS;

    case TEAM_UNJOINED:
    default:
        // Periodically clean stale discovered team entries
        uint32_t now = millis();
        for (auto it = discoveredTeams.begin(); it != discoveredTeams.end(); ) {
            if (now - it->lastSeenMs > DISCOVERED_TEAM_STALE_MS) {
                LOG_DEBUG("TeamMode: removing stale team 0x%08X", it->teamId);
                it = discoveredTeams.erase(it);
            } else {
                ++it;
            }
        }
        return BEACON_CHECK_INTERVAL_MS;
    }
}
