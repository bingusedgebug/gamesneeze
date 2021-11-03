#include "../../includes.hpp"
#include "features.hpp"

void bhop(CUserCmd *cmd) {
    if (CONFIGBOOL("Misc>Misc>Movement>JumpBug") &&
         (Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>JumpBug Key"))))
        return;
    if (CONFIGBOOL("Misc>Misc>Movement>Auto Hop")) {
        if (Globals::localPlayer->moveType() == 9) return;
        if (CONFIGBOOL("Misc>Misc>Movement>Humanised Bhop")) {
            // https://www.unknowncheats.me/forum/counterstrike-global-offensive/333797-humanised-bhop.html
            static int hopsRestricted = 0;
            static int hopsHit = 0;
            if (!(Globals::localPlayer->flags() & FL_ONGROUND)) {
                cmd->buttons &= ~IN_JUMP;
                hopsRestricted = 0;
            } else if ((rand() % 100 > CONFIGINT("Misc>Misc>Movement>Bhop Hitchance") &&
                            hopsRestricted <
                                 CONFIGINT("Misc>Misc>Movement>Bhop Max Misses")) ||
                 (CONFIGINT("Misc>Misc>Movement>Bhop Max Hops Hit") > 0 &&
                      hopsHit > CONFIGINT("Misc>Misc>Movement>Bhop Max Hops Hit"))) {
                cmd->buttons &= ~IN_JUMP;
                hopsRestricted++;
                hopsHit = 0;
            } else {
                hopsHit++;
            }
        } else {
            if (!(Globals::localPlayer->flags() & FL_ONGROUND)) {
                cmd->buttons &= ~IN_JUMP;
            }
        }
    }
}

void edgeJump(CUserCmd *cmd) {
    if (CONFIGBOOL("Misc>Misc>Movement>Edge Jump") &&
         Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>Edge Jump Key")) &&
         Features::Movement::flagsBackup & FL_ONGROUND &&
         !(Globals::localPlayer->flags() & FL_ONGROUND))
        cmd->buttons |= IN_JUMP;
}

void jumpBug(CUserCmd *cmd) {
    static bool shouldSkip = false;
    if (shouldSkip) {
        shouldSkip = false;
        return;
    }
    if (CONFIGBOOL("Misc>Misc>Movement>JumpBug") &&
         Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>JumpBug Key")) &&
         !(Features::Movement::flagsBackup & FL_ONGROUND ||
              Features::Movement::flagsBackup & FL_PARTIALGROUND) &&
         (Globals::localPlayer->flags() & FL_ONGROUND ||
              Globals::localPlayer->flags() & FL_PARTIALGROUND)) {
        cmd->buttons |= IN_DUCK;
        cmd->buttons &= ~IN_JUMP;
        shouldSkip = true;
    }
}

bool checkEdgebug() {
    static ConVar *sv_gravity = Interfaces::convar->FindVar("sv_gravity");
    float edgebugZVel =
         (sv_gravity->GetFloat() * 0.5f * Interfaces::globals->interval_per_tick);

    return Features::Movement::velBackup.z < -edgebugZVel &&
         round(Globals::localPlayer->velocity().z) == -round(edgebugZVel) &&
         Globals::localPlayer->moveType() != MOVETYPE_LADDER;
}

void Features::Movement::prePredCreateMove(CUserCmd *cmd) {
    if (!Globals::localPlayer) return;

    flagsBackup = Globals::localPlayer->flags();
    velBackup = Globals::localPlayer->velocity();

    bhop(cmd);

    if (shouldEdgebug && shouldDuckNext) cmd->buttons |= IN_DUCK;
}

void Features::Movement::postPredCreateMove(CUserCmd *cmd) {
    if (!Globals::localPlayer || Globals::localPlayer->moveType() == MOVETYPE_LADDER ||
         Globals::localPlayer->moveType() == MOVETYPE_NOCLIP)
        return;

    edgeJump(cmd);
    jumpBug(cmd);
}

void Features::Movement::edgeBugPredictor(CUserCmd *cmd) {
    if (!CONFIGBOOL("Misc>Misc>Movement>EdgeBug") ||
         !Menu::CustomWidgets::isKeyDown(CONFIGINT("Misc>Misc>Movement>EdgeBug Key")) ||
         !Globals::localPlayer->health()) {
        shouldEdgebug = false;
        return;
    }

    struct MovementVars {
        QAngle viewangles;
        QAngle view_delta;
        float forwardmove;
        float sidemove;
        int buttons;
    };
    static MovementVars backup_move;
    MovementVars original_move;
    original_move.viewangles = cmd->viewangles;
    original_move.view_delta = (cmd->viewangles - Globals::oldViewangles);
    original_move.forwardmove = cmd->forwardmove;
    original_move.sidemove = cmd->sidemove;
    original_move.buttons = cmd->buttons;
    if (!shouldEdgebug) backup_move = original_move;

    QAngle curAngles;

    int nCmdsPred = Interfaces::prediction->Split->nCommandsPredicted;

    int predCound = 0;
    int predCap = CONFIGINT("Misc>Misc>Movement>EdgeBug TotalPredCap");
    float highestGround = 0.f;
    int searchDir = 0;
    int lastPredGround = 0;
    int predictAmount = CONFIGINT("Misc>Misc>Movement>EdgeBug SinglePredCap");
    for (int t = 0; predCound < predCap; t++) {
        Features::Prediction::restoreEntityToPredictedFrame(nCmdsPred - 1);
        velBackup = Globals::localPlayer->velocity();

        static int lastType = 0;
        if(shouldEdgebug)
            t = lastType;
        
        bool doStrafe = t < 2 || t > 3;
        bool doDuck = t == 1 || t == 3;
        if(t > 3) {
            if(lastPredGround < 2)
                break;
            backup_move.view_delta += (backup_move.view_delta/2) * searchDir;
        }

        //cmd->viewangles = backup_move.viewangles;
        curAngles = backup_move.viewangles;

        for (int i = 0; i < predictAmount && predCound < predCap; i++) {
            if (doStrafe) {
                //cmd->viewangles += backup_move.view_delta;
                curAngles += backup_move.view_delta;
                cmd->forwardmove = backup_move.forwardmove;
                cmd->sidemove = backup_move.sidemove;
                auto viewbackup = cmd->viewangles;
                cmd->viewangles = curAngles;
                startMovementFix(cmd);
                cmd->viewangles = viewbackup;
                endMovementFix(cmd);
            } else {
                cmd->forwardmove = 0.f;
                cmd->sidemove = 0.f;
            }
            if (doDuck)
                cmd->buttons |= IN_DUCK;
            else
                cmd->buttons &= ~IN_DUCK;

            Features::Prediction::start(cmd);
            shouldEdgebug = checkEdgebug();
            velBackup = Globals::localPlayer->velocity();
            edgebugPos = Globals::localPlayer->origin();
            Features::Prediction::end();
            predCound++;
            if(!shouldEdgebug && t > 3 && Globals::localPlayer->origin().z < highestGround) {
                searchDir = -1;
                break;
            }
            if (Globals::localPlayer->flags() & FL_ONGROUND) {
                if(t == 0)
                    highestGround = Globals::localPlayer->origin().z;
                if(t == 2)
                    searchDir = Globals::localPlayer->origin().z < highestGround ? -1 : 1;
                if(t > 3) {
                    searchDir = 1;
                    if(Globals::localPlayer->origin().z < highestGround)
                        searchDir = -1;
                    else
                        highestGround = Globals::localPlayer->origin().z;
                }
                lastPredGround = i;
                break;
            }
            if(Globals::localPlayer->moveType() == MOVETYPE_LADDER)
                break;
            if (shouldEdgebug) {
                if(t < 4)
                    lastType = t;
                else
                    lastType = 0;
                shouldDuckNext = doDuck;
                if (doStrafe) {
                    cmd->forwardmove = backup_move.forwardmove;
                    cmd->sidemove = backup_move.sidemove;
                    cmd->viewangles = backup_move.viewangles + backup_move.view_delta;
                    backup_move.viewangles = cmd->viewangles;
                }
                if (i == 1)
                    Interfaces::engine->ExecuteClientCmd(
                         "play buttons/blip1.wav");  // TODO: play sound via a better method
                return;
            }
        }
    }

    cmd->viewangles = original_move.viewangles;
    cmd->forwardmove = original_move.forwardmove;
    cmd->sidemove = original_move.sidemove;
    cmd->buttons = original_move.buttons;
}

void Features::Movement::draw() {
    if (Features::Movement::shouldEdgebug) {
        Globals::drawList->AddText(
             ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("EdgeBug").x / 2) + 1,
                  (Globals::screenSizeY / 2) + 31),
             ImColor(0, 0, 0, 255), "EdgeBug");
        Globals::drawList->AddText(
             ImVec2((Globals::screenSizeX / 2) - (ImGui::CalcTextSize("EdgeBug").x / 2),
                  (Globals::screenSizeY / 2) + 30),
             ImColor(255, 255, 255, 255), "EdgeBug");

        Vector edgebugPos2D;
        if (worldToScreen(edgebugPos, edgebugPos2D)) {
            Globals::drawList->AddText(
                 ImVec2(edgebugPos2D.x - (ImGui::CalcTextSize("gaming").x / 2) + 1,
                      edgebugPos2D.y + 1),
                 ImColor(0, 0, 0, 255), "gaming");
            Globals::drawList->AddText(
                 ImVec2(edgebugPos2D.x - (ImGui::CalcTextSize("gaming").x / 2),
                      edgebugPos2D.y),
                 ImColor(255, 255, 255, 255), "gaming");
        }
    }
}