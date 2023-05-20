#pragma once
#include <cstdint>

constexpr uint16_t invalid_entity = -1;
struct Entity
{
  Entity() = default;
  Entity(uint32_t color, float x, float y, uint16_t eid) : color(color), x(x), y(y), eid(eid) {}

  uint32_t color = 0xff00ffff;
  float x = 0.f;
  float y = 0.f;
  float radius = 15.f;
  uint16_t eid = invalid_entity;

  // For AI-controlled entities
  float target_x = 0.f;
  float target_y = 0.f;
};

