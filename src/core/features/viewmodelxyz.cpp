#include "features.hpp"

void Features::ViewModelXYZ::createMove() {
    if (!CONFIGBOOL("Visuals>World>World>ViewmodelXYZ")) return;

    static ConVar* viewmodel_offset_x = Interfaces::convar->FindVar("viewmodel_offset_x");
    static ConVar* viewmodel_offset_y = Interfaces::convar->FindVar("viewmodel_offset_y");
    static ConVar* viewmodel_offset_z = Interfaces::convar->FindVar("viewmodel_offset_z");

    static bool doneOnceOff = false;
    if (!doneOnceOff) {
        viewmodel_offset_x->fnChangeCallback = 0;
        viewmodel_offset_y->fnChangeCallback = 0;
        viewmodel_offset_z->fnChangeCallback = 0;
        doneOnceOff = true;
    }

    if (viewmodel_offset_x->GetFloat() !=
         (CONFIGINT("Visuals>World>World>Viewmodel X") / 10.f))
        viewmodel_offset_x->SetValue(CONFIGINT("Visuals>World>World>Viewmodel X") / 10.f);

    if (viewmodel_offset_y->GetFloat() !=
         (CONFIGINT("Visuals>World>World>Viewmodel Y") / 10.f))
        viewmodel_offset_y->SetValue(CONFIGINT("Visuals>World>World>Viewmodel Y") / 10.f);

    if (viewmodel_offset_z->GetFloat() !=
         (CONFIGINT("Visuals>World>World>Viewmodel Z") / 10.f))
        viewmodel_offset_z->SetValue(CONFIGINT("Visuals>World>World>Viewmodel Z") / 10.f);
}