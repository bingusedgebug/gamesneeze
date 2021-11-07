#include "../../includes.hpp"
#include "features.hpp"

void Features::SkinChanger::frameStageNotify(FrameStage frame) {
    if (frame == FRAME_NET_UPDATE_POSTDATAUPDATE_START && Globals::localPlayer && Interfaces::engine->IsInGame() &&
         Globals::localPlayer->health() > 0) {
        Weapon* curWeapon =
             (Weapon*)Interfaces::entityList->GetClientEntity((uintptr_t)Globals::localPlayer->activeWeapon() & 0xFFF);
        if (curWeapon && curWeapon->itemIndex() != ItemIndex::INVALID) {
            if (curWeapon->clientClass()->m_ClassID == CKnife) {
                *curWeapon->modelIndex_ptr() = Interfaces::modelInfo->GetModelIndex("models/weapons/v_knife_css.mdl");
                *curWeapon->itemIndex_ptr() = WEAPON_KNIFE_CSS;
                Entity* viewmodel =
                     (Entity*)Interfaces::entityList->GetClientEntity((uintptr_t)Globals::localPlayer->viewmodel() & 0xFFF);
                if (!viewmodel) return;
                *viewmodel->modelIndex_ptr() = Interfaces::modelInfo->GetModelIndex("models/weapons/v_knife_css.mdl");
            }
        }
        for (size_t i = 0; Globals::localPlayer->getWeapons_ptr()[i] != (int)0xFFFFFFFF; i++) {
            Weapon* weapon = (Weapon*)Interfaces::entityList->GetClientEntity(
                 Globals::localPlayer->getWeapons_ptr()[i] & 0xFFF);  // GetClientEntityFromHandle is being gay
            if (weapon && weapon->itemIndex() != ItemIndex::INVALID &&
                 itemIndexMap.find(weapon->itemIndex()) != itemIndexMap.end()) {
                const char* weaponName = itemIndexMap.at(weapon->itemIndex());

                char buf[256];
                snprintf(buf, 256, "Misc>Skins>Skins>%s>PaintKit", weaponName);

                char buf2[256];
                snprintf(buf2, 256, "Misc>Skins>Skins>%s>Wear", weaponName);

                char buf3[256];
                snprintf(buf3, 256, "Misc>Skins>Skins>%s>StatTrack", weaponName);

                int paintkit = CONFIGINT(buf);
                float wear = (float)CONFIGINT(buf2) / 100.f;
                int statTrack = CONFIGINT(buf3);

                player_info_t info;
                Interfaces::engine->GetPlayerInfo(Interfaces::engine->GetLocalPlayer(), &info);
                *weapon->accountID_ptr() = info.xuid;
                *weapon->itemIDHigh_ptr() = -1;
                *weapon->paintKit_ptr() = paintkit;
                *weapon->wear_ptr() = wear;
                *weapon->statTrack_ptr() = statTrack;
            }
        }
    }
}