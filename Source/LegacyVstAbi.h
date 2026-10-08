#pragma once
// Original declarations of the legacy plugin binary interface. No SDK headers.
#include <cstdint>
#include <cstddef>
namespace legacy {
struct Effect;
using Callback = intptr_t (*) (Effect*, int32_t, int32_t, intptr_t, void*, float);
using Process = void (*) (Effect*, float**, float**, int32_t);
struct Effect {
    int32_t magic;
    Callback dispatcher;
    Process process;
    void (*setParameter)(Effect*, int32_t, float);
    float (*getParameter)(Effect*, int32_t);
    int32_t numPrograms, numParams, numInputs, numOutputs, flags;
    intptr_t reserved1, reserved2;
    int32_t initialDelay, realQualities, offQualities;
    float ioRatio;
    void* object;
    void* user;
    int32_t uniqueID, version;
    Process processReplacing;
    void (*processDoubleReplacing)(Effect*, double**, double**, int32_t);
    char future[56];
};
struct Rect { int16_t top, left, bottom, right; };
static_assert(sizeof(Effect) == (sizeof(void*) == 8 ? 192 : 144), "Legacy ABI layout");
static_assert(offsetof(Effect, object) == (sizeof(void*) == 8 ? 96 : 64), "Legacy ABI alignment");
}
