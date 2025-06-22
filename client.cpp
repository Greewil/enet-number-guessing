#include <iostream>
#include <enet/enet.h>


#define CLIENT_VERSION "0.1.1"


int main(int argc, char ** argv) {
  if (enet_initialize() != 0) {
    fprintf(stderr, "An error occured while initializing ENet!\n");
    return EXIT_FAILURE;
  }
  atexit(enet_deinitialize);

  printf("Starting client (version %s)\n", CLIENT_VERSION);

  ENetHost* client;
  client = enet_host_create(NULL	/* the address to bind the server host to */,
                            1	/* allow up to 32 clients and/or outgoing connections */,
                            1	/* allow up to 1 channel to be used, 0. */,
                            0	/* assume any amount of incoming bandwidth */,
                            0	/* assume any amount of outgoing bandwidth */);
  if (client == NULL) {
    fprintf(stderr, "An error occurred while trying to create an ENet client host.\n");
    exit (EXIT_FAILURE);
  }

  ENetAddress address;
  ENetEvent event;
  ENetPeer* peer;

  // TODO try to get address and port from args (or from console input)
  enet_address_set_host(&address, "127.0.0.1");
  address.port = 7777;

  peer = enet_host_connect(client, &address, 1, 0);
  if (peer == NULL) {
    fprintf(stderr, "No available peers for initiating an ENet connection!\n");
    return EXIT_FAILURE;
  }

  if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
    puts("Connection to 127.0.0.1:7777 succeeded.");
  } else {
    enet_peer_reset(peer);
    puts("Connection to 127.0.0.1:7777 failed.");
    return EXIT_SUCCESS;
  }

  // [...Game Loop...]

  while (enet_host_service(client, &event, 1000) > 0) {
    switch (event.type) {
      case ENET_EVENT_TYPE_RECEIVE:
        printf("A packet of length %u containing %s was received from %x:%u on channel %u.\n",
               event.packet -> dataLength,
               event.packet -> data,
               event.peer -> address.host,
               event.peer -> address.port,
               event.channelID);
        break;
    }
  }

  // [...end game loop...]

  enet_peer_disconnect(peer, 0);

  while (enet_host_service(client, &event, 3000) > 0) {
    switch (event.type) {
      case ENET_EVENT_TYPE_RECEIVE:
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        puts("Disconnection succeeded.");
        break;
      case ENET_EVENT_TYPE_NONE:
        puts("Disconnection succeeded.");
        break;
    }
  }

  return EXIT_SUCCESS;
}
