#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <cmath>

static const size_t kAiEntities = 8;
static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;

float random_coord_on_map() {
  return (rand() % 4) * 200.f - 300.0f;
}

Entity& spawn_new_entity(uint16_t eid, ENetPeer *peer)
{
  uint32_t color = 0xff000000 +
                   0x00440000 * (rand() % 5) +
                   0x00004400 * (rand() % 5) +
                   0x00000044 * (rand() % 5);
  float x = random_coord_on_map();
  float y = random_coord_on_map();
  Entity& ent = entities.emplace_back(color, x, y, eid);

  controlledMap[eid] = peer;

  return ent;
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  
  Entity& ent = spawn_new_entity(newEid, peer);

  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f;
  deserialize_entity_state(packet, eid, x, y);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
    }
}

void spawn_ai_entities()
{
  for (size_t i = 0; i < kAiEntities; ++i)
  {
    Entity& entity = spawn_new_entity(i, nullptr);
    entity.radius = (rand() % 3) * 5.f + 5.0f;
  }
}

void move_ai_entities(float dt)
{
  for (auto& entity : entities) {
    if (controlledMap[entity.eid] == nullptr) {
      float v_x = entity.target_x - entity.x;
      float v_y = entity.target_y - entity.y;

      float length = std::sqrtf(v_x * v_x + v_y * v_y);
      if (length != 0.0f) {
        v_x = 100.0f * v_x / length;
        v_y = 100.0f * v_y / length;
      }

      entity.x += v_x * dt;
      entity.y += v_y * dt;

      float to_target = (entity.target_x - entity.x) * (entity.target_x - entity.x) +
                        (entity.target_y - entity.y) * (entity.target_y - entity.y);

      if (to_target < 0.001f) {
        entity.radius = (rand() % 3) * 5.f + 5.0f;

        entity.target_x = random_coord_on_map();
        entity.target_y = random_coord_on_map();
      }
    }
  }
}

void check_collisions() {
  for (auto& first : entities)
  {
    for (auto& second : entities)
    {
      if (first.eid == second.eid) { continue; }

      auto* small = &first;
      auto* big = &second;
      if (big->radius < small->radius)
      {
        small = &second;
        big = &first;
      }

      float distance = (big->x - small->x) * (big->x - small->x) +
                       (big->y - small->y) * (big->y - small->y);

      if (distance <= big->radius * big->radius)
      {
        small->radius /= 2.0f;
        big->radius += small->radius;

        small->x = random_coord_on_map();
        small->y = random_coord_on_map();

        if (controlledMap[small->eid] != nullptr)
        {
          send_snapshot(controlledMap[small->eid], small->eid, small->x, small->y, small->radius);
        }
        else
        {
          small->target_x = random_coord_on_map();
          small->target_y = random_coord_on_map();
        }

        if (controlledMap[big->eid] != nullptr)
        {
          send_snapshot(controlledMap[big->eid], big->eid, big->x, big->y, big->radius);
        }
      }
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
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  spawn_ai_entities();

  clock_t time_start = clock();
  while (true)
  {
    float dt = (0.0f + clock() - time_start) / CLOCKS_PER_SEC;
    time_start = clock();

    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    move_ai_entities(dt);
    check_collisions();

    static int t = 0;
    for (const Entity &e : entities)
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        if (controlledMap[e.eid] != peer)
          send_snapshot(peer, e.eid, e.x, e.y, e.radius);
      }
    //usleep(400000);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


