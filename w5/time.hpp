#pragma once

#include <cstdint>

constexpr uint32_t kServerUpdatesPerSecond = 32;
constexpr uint32_t kServerFixedTimeStep    = (1000.0f / kServerUpdatesPerSecond);  // In ms
constexpr float    kServerFixedTimeStepF   = kServerFixedTimeStep;                 // In ms