#include "protocol.h"
#include <cstring> // memcpy
#include "bitstream.hpp"

void send_join(ENetPeer *peer)
{
  Bitstream bitstream;
  bitstream.Write(E_CLIENT_TO_SERVER_JOIN);

  ENetPacket *packet = enet_packet_create(nullptr, bitstream.Size(), ENET_PACKET_FLAG_RELIABLE);
  bitstream.Read(packet->data, bitstream.Size());

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  Bitstream bitstream;
  bitstream.Write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bitstream.Write(ent);

  ENetPacket *packet = enet_packet_create(nullptr, bitstream.Size(), ENET_PACKET_FLAG_RELIABLE);
  bitstream.Read(packet->data, bitstream.Size());

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  Bitstream bitstream;
  bitstream.Write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  bitstream.Write(eid);

  ENetPacket *packet = enet_packet_create(nullptr, bitstream.Size(), ENET_PACKET_FLAG_RELIABLE);
  bitstream.Read(packet->data, bitstream.Size());

  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
  Bitstream bitstream;
  bitstream.Write(E_CLIENT_TO_SERVER_STATE);
  bitstream.Write(eid);
  bitstream.Write(x);
  bitstream.Write(y);

  ENetPacket *packet = enet_packet_create(nullptr, bitstream.Size(), ENET_PACKET_FLAG_UNSEQUENCED);
  bitstream.Read(packet->data, bitstream.Size());

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float radius)
{
  Bitstream bitstream;
  bitstream.Write(E_SERVER_TO_CLIENT_SNAPSHOT);
  bitstream.Write(eid);
  bitstream.Write(x);
  bitstream.Write(y);
  bitstream.Write(radius);

  ENetPacket *packet = enet_packet_create(nullptr, bitstream.Size(), ENET_PACKET_FLAG_UNSEQUENCED);
  bitstream.Read(packet->data, bitstream.Size());

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  Bitstream bitstream{packet->data, packet->dataLength};
  bitstream.Skip<MessageType>();
  bitstream.Read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  Bitstream bitstream{packet->data, packet->dataLength};
  bitstream.Skip<MessageType>();
  bitstream.Read(eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
  Bitstream bitstream{packet->data, packet->dataLength};
  bitstream.Skip<MessageType>();
  bitstream.Read(eid);
  bitstream.Read(x);
  bitstream.Read(y);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &radius)
{
  Bitstream bitstream{packet->data, packet->dataLength};
  bitstream.Skip<MessageType>();
  bitstream.Read(eid);
  bitstream.Read(x);
  bitstream.Read(y);
  bitstream.Read(radius);
}

