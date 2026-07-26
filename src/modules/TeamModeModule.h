#pragma once

#include "concurrency/OSThread.h"
#include "mesh/NodeDB.h"

// Team state machine
enum TeamState {
    TEAM_UNJOINED,
    TEAM_LEADER,
    TEAM_MEMBER
};

// Discovered team entry (kept in RAM only, reset on reboot)
struct DiscoveredTeam {
    uint32_t teamId;
    uint32_t leaderNodeNum;
    uint32_t lastSeenMs;
};

class TeamModeModule : private concurrency::OSThread
{
  public:
    TeamModeModule();

    // UI query methods
    TeamState getState() const { return state; }
    uint32_t getTeamId() const { return teamId; }
    uint32_t getLeaderNodeNum() const { return leaderNodeNum; }
    bool isDisconnected() const { return disconnected; }
    bool isAlertSilenced() const { return alertSilenced; }
    uint32_t getLastBeaconAge() const;
    float getLastBeaconSNR() const { return lastBeaconSNR; }
    int32_t getLastBeaconRSSI() const { return lastBeaconRSSI; }
    uint32_t getBroadcastIntervalMs() const { return broadcastIntervalMs; }

    // Discovered teams list
    const std::vector<DiscoveredTeam> &getDiscoveredTeams() { return discoveredTeams; }
    void clearDiscoveredTeams() { discoveredTeams.clear(); }

    // Scanning
    void startScan();
    void stopScan() { scanning = false; }
    bool isScanning() const { return scanning; }
    uint32_t getScanRemainingMs() const;

    // Actions
    void createTeam();
    void joinTeam(uint32_t tId, uint32_t leaderNode);
    void leaveTeam();
    void silenceAlert();
    void setBroadcastInterval(uint32_t ms) { broadcastIntervalMs = ms; }

    // Packet handler (called from handleReceived)
    void handleTeamPacket(const meshtastic_MeshPacket *p);

  protected:
    int32_t runOnce() override;

  private:
    void broadcastBeacon();
    void sendJoinRequest(uint32_t tId, uint32_t leaderNode);
    void playDisconnectedTone();
    void showDisconnectedBanner();

    // Volatile state (all reset on reboot)
    static TeamState state;
    static uint32_t teamId;
    static uint32_t leaderNodeNum;
    static uint32_t lastBeaconTimeMs;
    static uint32_t lastBroadcastTimeMs;
    static bool disconnected;
    static bool alertSilenced;
    static uint32_t broadcastIntervalMs;        // default 15000ms
    static std::vector<DiscoveredTeam> discoveredTeams;
    static bool scanning;
    static uint32_t scanStartTimeMs;
    static float lastBeaconSNR;
    static int32_t lastBeaconRSSI;
};

extern TeamModeModule *teamModeModule;
