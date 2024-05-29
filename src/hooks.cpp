#include "hooks.h"

namespace hooks
{
    float                g_marker_x = 0.f;
    float                g_marker_y = 0.f;
    RE::TESQuestTarget** g_target = nullptr;

    int c = 0xbeef;

    void QuestUpdateHook()
    {
        if (g_target &&
            mapmarker::Manager::GetSingleton()->GetState() == mapmarker::Manager::State::kWaiting)
        {
            auto real_target = *(g_target);
            mapmarker::Manager::GetSingleton()->ProcessCompassMarker(
                real_target, { g_marker_x, g_marker_y });
        }
    }

    // Call inside the compass update function that updates quest markers
    uintptr_t UpdateQuestHookedFuncAddr = 0;
    auto      UpdateQuestHookLoc = RelocAddr<uintptr_t>(0x8b2bd0 + 0x13A);
    auto      UpdateQuestHookedFunc = RelocAddr<uintptr_t>(0x8b3430);

    void InstallUpdateQuest()
    {
        UpdateQuestHookedFuncAddr = UpdateQuestHookedFunc.GetUIntPtr();
        {
            struct Code : Xbyak::CodeGenerator
            {
                Code()
                {
                    Xbyak::Label hookLabel;
                    Xbyak::Label retnLabel;

                    // original quest compass update function
                    mov(rax, UpdateQuestHookedFuncAddr);
                    call(rax);

                    // After it returns, the quest target is in rbx and the marker pos is at rsp-0x78
                    // I don't know how to refer to these and stop them from being clobbered so I save
                    push(rax);

                    movss(xmm0, ptr[rsp - 0x70]);
                    mov(rax, (uintptr_t)&g_marker_x);
                    movss(ptr[rax], xmm0);

                    movss(xmm0, ptr[rsp - 0x6C]);
                    mov(rax, (uintptr_t)&g_marker_y);
                    movss(ptr[rax], xmm0);

                    mov(rax, (uintptr_t)&g_target);
                    mov(ptr[rax], rbx);

                    call(ptr[rip + hookLabel]);

                    pop(rax);

                    jmp(ptr[rip + retnLabel]);

                    L(hookLabel), dq(reinterpret_cast<uintptr_t>(&QuestUpdateHook));
                    L(retnLabel), dq(UpdateQuestHookLoc + 5);

                    ready();
                }
            };
            Code code;
            code.ready();

            auto& trampoline = SKSE::GetTrampoline();

            trampoline.write_branch<5>(UpdateQuestHookLoc.GetUIntPtr(), trampoline.allocate(code));

            SKSE::log::trace("Quest Update hook complete");
        }
    }

    void Install() { InstallUpdateQuest(); }
}