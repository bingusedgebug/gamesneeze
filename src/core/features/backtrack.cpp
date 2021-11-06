#include <cfloat>
#include <cstring>

#include "../../includes.hpp"
#include "features.hpp"

bool Features::Backtrack::isRecordValid(float simtime) {
    auto net = Interfaces::engine->GetNetChannelInfo();
    if (!net) return false;

    static ConVar *sv_maxunlag = Interfaces::convar->FindVar("sv_maxunlag");
    static ConVar *sv_client_min_interp_ratio = Interfaces::convar->FindVar("sv_client_min_interp_ratio");
    static ConVar *sv_client_max_interp_ratio = Interfaces::convar->FindVar("sv_client_max_interp_ratio");
    static ConVar *cl_updaterate = Interfaces::convar->FindVar("cl_updaterate");
    static ConVar *sv_maxupdaterate = Interfaces::convar->FindVar("sv_maxupdaterate");
    static ConVar *cl_interp = Interfaces::convar->FindVar("cl_interp");
    static ConVar *cl_interp_ratio = Interfaces::convar->FindVar("cl_interp_ratio");

    // ty masterlooser
    // https://www.unknowncheats.me/forum/counterstrike-global-offensive/359885-fldeadtime-int.html
    if (simtime < floorf(Interfaces::globals->curtime - sv_maxunlag->GetFloat())) return false;

    float lerp = std::max(cl_interp->GetFloat(),
                          ((std::clamp(cl_interp_ratio->GetFloat(), sv_client_min_interp_ratio->GetFloat(),
                                       sv_client_max_interp_ratio->GetFloat()))) /
                              ((sv_maxupdaterate->GetFloat()) ? sv_maxupdaterate->GetFloat() : cl_updaterate->GetFloat()));

    auto delta = std::clamp(net->GetLatency(0) + net->GetLatency(1) + lerp, 0.f, sv_maxunlag->GetFloat()) -
                 (TICKS_TO_TIME(Globals::localPlayer->tickbase()) - simtime);
    return fabsf(delta) <= sv_maxunlag->GetFloat();
}

void Features::Backtrack::store(CUserCmd *cmd) {
    if (CONFIGBOOL("Legit>Backtrack>Backtrack") && cmd->tick_count != 0 && Interfaces::engine->IsInGame() &&
        Globals::localPlayer) {
        // Store
        BacktrackTick currentTick;
        for (int i = 1; i < Interfaces::globals->maxClients; i++) {
            Player *p = (Player *)Interfaces::entityList->GetClientEntity(i);
            if (p) {
                if (p->health() > 0 && !p->dormant() && p != Globals::localPlayer && p->isEnemy()) {
                    BacktrackPlayer player;
                    player.playerIndex = i;
                    player.playerFlags = p->flags();
                    player.playerVelocity = p->velocity().Length2D();
                    player.playerSimTime = p->simtime();
                    player.playerOrigin = p->origin();
                    if (p->getAnythingBones(player.boneMatrix)) {
                        currentTick.players.insert(std::pair<int, BacktrackPlayer>(i, player));
                    }
                    player.playerHeadPos =
                        Vector(player.boneMatrix[8][0][3], player.boneMatrix[8][1][3], player.boneMatrix[8][2][3]);
                } else {
                    if (currentTick.players.find(i) != currentTick.players.end()) {
                        currentTick.players.erase(i);
                    }
                }
            }
        }
        currentTick.tickCount = cmd->tick_count;
        backtrackTicks.insert(backtrackTicks.begin(), currentTick);

        // Delete ticks we cant backtrack
        static ConVar *sv_maxunlag = Interfaces::convar->FindVar("sv_maxunlag");
        while (backtrackTicks.size() > static_cast<size_t>(TIME_TO_TICKS(sv_maxunlag->GetFloat()))) backtrackTicks.pop_back();

        // Delete records we can't backtrack
        float max_dist = (float)CONFIGINT("Legit>Backtrack>Backtrack Distance");
        if (max_dist == 69.f) max_dist = FLT_MAX;
        for (int tick = (int)backtrackTicks.size() - 1; tick > 0; tick--) {
            for (auto player : backtrackTicks.at(tick).players) {
                Player *p = (Player *)Interfaces::entityList->GetClientEntity(player.second.playerIndex);
                if (!p || !(p->health()) || p->dormant() || !isRecordValid(player.second.playerSimTime) ||
                    (player.second.playerOrigin - p->origin()).Length() > max_dist) {
                    backtrackTicks.at(tick).players.erase(player.first);
                }
            }
        }
    }
}

void Features::Backtrack::createMove(CUserCmd *cmd) {
    if (CONFIGBOOL("Legit>Backtrack>Backtrack") && cmd->tick_count != 0 && Interfaces::engine->IsInGame() &&
        Globals::localPlayer) {
        // Find how far we should backtrack in this tick
        QAngle viewAngles = cmd->viewangles;
        //Interfaces::engine->GetViewAngles(viewAngles);
        viewAngles += Globals::localPlayer->aimPunch() * 2;

        float closestDelta = FLT_MAX;
        int closestTick = cmd->tick_count;

        if (cmd->buttons & (1 << 0)) {
            Vector localPlayerEyePos = Globals::localPlayer->eyePos();
            for (BacktrackTick tick : backtrackTicks) {
                for (auto player : tick.players) {
                    Vector targetEyePos = Vector(player.second.boneMatrix[8][0][3], player.second.boneMatrix[8][1][3],
                                                 player.second.boneMatrix[8][2][3]);  // 8 is headbone in bonematrix

                    QAngle angleToCurrentPlayer = calcAngle(localPlayerEyePos, targetEyePos);
                    angleToCurrentPlayer -= viewAngles;
                    if (angleToCurrentPlayer.y > 180.f) {
                        angleToCurrentPlayer.y -= 360.f;
                    }

                    if (angleToCurrentPlayer.Length() < closestDelta) {
                        closestDelta = angleToCurrentPlayer.Length();
                        closestTick = tick.tickCount;
                    }
                }
            }
        }
        lastBacktrack = cmd->tick_count - closestTick;  // To show how much you backtracked in hitlogs
        cmd->tick_count = closestTick;
    }
}
