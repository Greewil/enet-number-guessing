#include <iostream>
#include <cstring>
#include <bits/stdc++.h>

#include "enet/enet.h"


#define CLIENT_VERSION "0.1.2"


std::pair<std::string, int> getAddressAndPortFromSocket(std::string serverSocket) {
  std::stringstream ss(serverSocket);
  std::string serverAddress;
  std::string serverPort;
  getline(ss, serverAddress, ':');
  getline(ss, serverPort, ':');
  return std::pair<std::string, int>(serverAddress, std::stoi(serverPort));
}


int main(int argc, char** argv) {
  std::srand(std::time({}));

  if (enet_initialize() != 0) {
    fprintf(stderr, "An error occured while initializing ENet!\n");
    return EXIT_FAILURE;
  }
  atexit(enet_deinitialize);

  std::string serverSocket = "127.0.0.1:7777";
  if (argv[1] != NULL) {
    serverSocket = argv[1];
  }
  std::pair<std::string, int> addressAndPort = getAddressAndPortFromSocket(serverSocket);\

  int randNum = std::rand() % 1000;
  // int randNum = rand() % (max - min + 1) + min;
  std::string username = "tumba-yumba-" + std::to_string(randNum);
  if (argv[2] != NULL) {
    username = argv[2];
  }

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

  enet_address_set_host(&address, addressAndPort.first.c_str());
  address.port = addressAndPort.second;

  peer = enet_host_connect(client, &address, 1, 0);
  if (peer == NULL) {
    fprintf(stderr, "No available peers for initiating an ENet connection!\n");
    return EXIT_FAILURE;
  }

  if (enet_host_service(client, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
    printf("Connection to '%s:%i' succeeded.\n", addressAndPort.first.c_str(), addressAndPort.second);
  } else {
    enet_peer_reset(peer);
    printf("Connection to '%s:%i' failed.\n", addressAndPort.first.c_str(), addressAndPort.second);
    return EXIT_SUCCESS;
  }

  // [...Game Loop...]

  // First of all sending special text containing username
  // (so server will store this username in map and it will be associated with socket from which user connected)
  // later this username will be used in leaderboard

  /* Create a reliable packet of size setUsername.size() + 1 containing "${setUsername}\0" */
  std::string setUsername = "username:" + username;
  ENetPacket* packet = enet_packet_create(setUsername.c_str(),
                                          setUsername.size() + 1,
                                          ENET_PACKET_FLAG_RELIABLE);
  /* Send the packet to the peer over channel id 0. */
  enet_peer_send(peer, 0, packet);

  std::string currentGuess = "";
  ENetPacket* currentGuessPacket;
  while (true) {
    std::cin >> currentGuess;
    if (currentGuess == "exit") {
      break;
    }
    currentGuessPacket = enet_packet_create(currentGuess.c_str(),
                                            currentGuess.size() + 1,
                                            ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, currentGuessPacket);

    if (enet_host_service(client, &event, 1000) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_RECEIVE:
          printf("Guessed number was: %s.\n", event.packet -> data);
          /* Clean up the packet now that we're done using it. */
          enet_packet_destroy(event.packet);
          break;
      }
    } else {
      printf("connection lost ...\n");
      // TODO try to reconnect one time
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
