// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <deque>
#include <vector>
#include <unordered_map>
#include "entity.h"
#include "protocol.h"
#include "time.hpp"
#include <cmath>

static std::unordered_map<uint16_t, std::deque<EntitySnapshot>> entitySnapshots;
static std::unordered_map<uint16_t, uint32_t> lastUpdateTime;

static std::vector<InputSnapshot> playerInputSnapshots;

static uint32_t inputGen = 0;

static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;

bool FloatsEqual(float a, float b) {
  return std::fabs(a - b) < 0.001f;
}

void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  // TODO: Direct adressing, of course!
  for (const Entity &e : entities)
    if (e.eid == newEntity.eid)
      return; // don't need to do anything, we already have entity
  entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void local_simulation_rollback(const EntitySnapshot& snapshot)
{
  Entity& e = entities[my_entity];
  e.x = snapshot.x;
  e.y = snapshot.y;
  e.ori = snapshot.ori;
  e.gen = snapshot.gen;

  size_t inputs_size = playerInputSnapshots.size();
  if (inputs_size > 0) {
    size_t idx = inputs_size - 1;
    for (; idx > 0 && playerInputSnapshots[idx].gen >= snapshot.gen; --idx) { ; }

    for (; idx < inputs_size; ++idx) {
      e.thr = playerInputSnapshots[idx].thr;
      e.steer = playerInputSnapshots[idx].steer;

      simulate_entity(e, playerInputSnapshots[idx].dt);
    }
  }
}

void on_snapshot(ENetPacket *packet)
{
  EntitySnapshot snapshot{};
  deserialize_snapshot(packet, snapshot);

  auto& snapshots = entitySnapshots[snapshot.eid];

  if (snapshots.empty() || snapshots.back().gen < snapshot.gen) {
    snapshots.push_back(snapshot);
  }

  lastUpdateTime[snapshot.eid] = enet_time_get();

  // TODO: Direct adressing, of course!
  for (Entity &e : entities)
    if (e.eid == snapshot.eid)
    {
      if (e.eid == my_entity) {
        if (!FloatsEqual(e.x, snapshot.x) || !FloatsEqual(e.y, snapshot.y) || !FloatsEqual(e.ori, snapshot.ori)) {
          local_simulation_rollback(snapshot);
        }
        continue;
      }

      e.x = snapshot.x;
      e.y = snapshot.y;
      e.ori = snapshot.ori;
      e.gen = snapshot.gen;
    }
}

float lerp(float a, float b, float t) {
  return a + t * (b - a);
}

void interpolate_entities(uint32_t cur_time)
{
  for (auto& [eid, snapshots] : entitySnapshots)
  {
    if (eid == my_entity) { continue; }

    auto& entity = entities[eid];

    if (snapshots.size() > 2)
    {
      const auto& first = *(snapshots.end() - 2);
      const auto& second = *(snapshots.end() - 1);

      float t = (cur_time - lastUpdateTime[eid]) * (second.gen - first.gen) / kServerFixedTimeStepF;
      entity.x = lerp(first.x, second.x, t);
      entity.y = lerp(first.y, second.y, t);
      entity.ori = lerp(first.ori, second.ori, t);
    }
  }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  bool connected = false;
  uint32_t lastSimulationTime = enet_time_get();
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    uint32_t curTime = enet_time_get();
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        break;
      default:
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      uint32_t gens_passed = (curTime - lastSimulationTime) / kServerFixedTimeStep;
      if (gens_passed > 0) {
        lastSimulationTime = curTime;
      }

      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      // TODO: Direct adressing, of course!
      for (Entity &e : entities)
      {
        if (e.eid == my_entity)
        {
          // Update
          float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
          float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

          // Send
          InputSnapshot input{};
          input.eid = my_entity;
          input.input_num = inputGen++;
          input.thr = thr;
          input.steer = steer;
          input.dt = dt;
          input.gen = e.gen;
          send_entity_input(serverPeer, input);

          playerInputSnapshots.push_back(input);

          e.thr = thr;
          e.steer = steer;
          e.gen += gens_passed;
          simulate_entity(e, dt);
        }
      }
    }

    interpolate_entities(curTime);

    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        for (const Entity &e : entities)
        {
          const Rectangle rect = {e.x, e.y, 3.f, 1.f};
          DrawRectanglePro(rect, {0.f, 0.5f}, e.ori * 180.f / PI, GetColor(e.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
