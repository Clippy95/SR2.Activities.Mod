// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "BlingMenu_public.h"
struct vector3 {
    float x;
    float y;
    float z;
};

typedef char __cdecl vehicle_apply_damageT(
	void* vehicle_data,
	DWORD* attacker,
	void* attacking_vehicle_handle,
	int attacker_weapon,
	vector3* input_damage_position,
	int damage_normal,
	float damage_radius,
	int damage_to_apply,
	int from_type,
	vector3* damage_velocity,
	float additional_deform_radius,
	char damage_components_only,
	int no_packet,
	char no_callbacks);
vehicle_apply_damageT* vehicle_apply_damage = (vehicle_apply_damageT*)(0x00AAD3D0);

typedef void(__stdcall* character_force_never_dieT)(int player, unsigned __int8 arg4);
character_force_never_dieT character_force_invulnerable = (character_force_never_dieT)0x00965F40;

// Please stick to using in-game functions, to do any type of actions such as damage, 
// or modifying anything player/object related, as these functions handles packets and networking (CO-OP).
namespace Fraud {
    volatile float Adrenaline_Speed_Explode = 20.f;
    volatile bool human_velocity_multiplier_on_explode = false;
    // why
    volatile bool toggle_mod = true;
    void Adrenaline_Explosion(int vehicle, int esp, int player) {
        if (!vehicle || !esp || !player)
            return;
        float* vehicle_vel_mag_squared = (float*)(esp + 0x10);
        vector3* human_velocity = (vector3*)(esp + 0x2C);
        if (*vehicle_vel_mag_squared > Adrenaline_Speed_Explode) {
            float* vehicle_data = (float*)vehicle;
            vector3 vehicle_pos_maybe{};
            vehicle_pos_maybe.x = vehicle_data[5];
            vehicle_pos_maybe.y = vehicle_data[6];
            vehicle_pos_maybe.z = vehicle_data[7];
            vector3 damage_velocity{};
            damage_velocity.x = 0.f;
            damage_velocity.y = 0.f;
            damage_velocity.z = 0.f;
            // So a bit of an annoyance here but we have to force human to be invulnerable otherwise you'll die from the fire, ideally find where fireproof function is, cannot find it..
            // as for re-setting vulnerablity, game already handles it in fraud_level_instance_host::process_waiting_after_win_state()
            character_force_invulnerable(player, 1);
            vehicle_apply_damage((void*)vehicle, 0, 0, 0, &vehicle_pos_maybe, 0, 100.f, 0x7FFFFFFF, 1, &damage_velocity, 0.f, 0, 0, 0);
            if (human_velocity_multiplier_on_explode) {
                human_velocity->x *= 10.f;
                human_velocity->y *= 10.f;
                human_velocity->z *= 10.f;
            }
        }
    }
    void __declspec(naked) fraud_adrenaline_on_vehicle_hit_MidHook() {
        static int Continue = 0x006524FF;
         int vehicle;
         int esp_pointer;
         int player;
        __asm {
            mov vehicle, edi
            mov esp_pointer, esp
            mov player, ebp
        }
        __asm {
            pushad
            pushfd
        }
        if(toggle_mod)
        Adrenaline_Explosion(vehicle, esp_pointer, player);
        __asm {
            popfd
            popad
            fld dword ptr[esp + 10]
            fcomp ds : dword ptr[0x00DD071C]
            jmp Continue
        }
    }
};

DWORD WINAPI LateBM(LPVOID lpParameter)
{
    // Funny Tervel hooking Sleep, BlingMenu settings get added after 2450ms so 1500 is good enough.
    SleepEx(1500, 0);
    BlingMenuAddBool("Activities Mod", "Toggle Fraud modifications", (bool*)&Fraud::toggle_mod, nullptr);
    BlingMenuAddFloat("Activities Mod", "Internal Adrenaline Speed Explode", (float*)&Fraud::Adrenaline_Speed_Explode, nullptr, 0.5f, 0.1f, 40.f);
    BlingMenuAddBool("Activities Mod", "human_velocity_multiplier_on_explode", (bool*)&Fraud::human_velocity_multiplier_on_explode, nullptr);
    return 0;
}

void init() {
    Memory::VP::InjectHook(0x006524F5, &Fraud::fraud_adrenaline_on_vehicle_hit_MidHook, Memory::VP::HookType::Jump);
    if (BlingMenuLoad()) {
        CreateThread(0, 0, LateBM, 0, 0, 0);
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        init();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

