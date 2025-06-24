#include <iostream>
#include <cstring>
#include <bits/stdc++.h>

#include "enet/enet.h"



#define CLIENT_VERSION "0.2.1"



std::pair<std::string, int> getAddressAndPortFromSocket(std::string serverSocket) {
  std::stringstream ss(serverSocket);
  std::string serverAddress;
  std::string serverPort;
  getline(ss, serverAddress, ':');
  getline(ss, serverPort, ':');
  return std::pair<std::string, int>(serverAddress, std::stoi(serverPort));
}

bool isOnlyDigits(std::string str) {
  return std::all_of(
    str.begin(),
    str.end(),
    [](char x) { return std::isdigit(x); }
  );
}

void sendPackageToServer(ENetPeer* peer, const std::string& sendingData, ENetPacket* packet) {
  /* Create a reliable packet of size sendingData.size() + 1 containing "${sendingData}\0" */
  packet = enet_packet_create(sendingData.c_str(),
                              sendingData.size() + 1,
                              ENET_PACKET_FLAG_RELIABLE);
  /* Send the packet to the peer over channel id 0. */
  enet_peer_send(peer, 0, packet);
}

void printResponseFromServer(ENetPacket* packet) {
  printf("%s\n", packet->data);
  /* Clean up the packet now that we're done using it. */
  enet_packet_destroy(packet);
}

void showHelp() {
  printf("\nUSAGE: client_app [<server_address:server_port>] [username]\n");
  printf("\n");
  printf("commands after successful connection to server:\n");
  printf("      <int>  to guess the number.\n");
  printf("      help | h\n");
  printf("                to show client usage.\n");
  printf("      setusername:<new_name>\n");
  printf("                to update your username on server leaderboard.\n");
  printf("      leaderboard | lb\n");
  printf("                to show leaderboard.\n");
  printf("      exit     to disconnect and exit.\n");
  printf("\n");
}

void setUsername(ENetPeer* peer, const std::string& newUsername, ENetPacket* packet) {
  sendPackageToServer(peer, newUsername, packet);
}



int main(int argc, char** argv) {
  std::srand(std::time({}));

  if (enet_initialize() != 0) {
    fprintf(stderr, "An error occured while initializing ENet!\n");
    return EXIT_FAILURE;
  }
  atexit(enet_deinitialize);

  showHelp();

  std::string serverSocket = "127.0.0.1:7777";
  if (argv[1] != NULL) {
    serverSocket = argv[1];
  }
  std::pair<std::string, int> addressAndPort = getAddressAndPortFromSocket(serverSocket);\

  int randNum = std::rand() % 1000;
  // int randNum = rand() % (max - min + 1) + min;
  std::string username = "tumba-yumba-" + std::to_string(randNum);
  if (argv[1] != NULL && argv[2] != NULL) {
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
  enet_peer_timeout(peer, 100000, 0, 100000);

  // [...Game Loop...]

  std::string currentInput = "";
  ENetPacket* currentPacket;
  bool isForceDisconnect = false;

  // First of all sending special text containing username
  // (so server will store this username in map and it will be associated with socket from which user connected)
  // later this username will be used in leaderboard
  setUsername(peer, "setusername:" + username, currentPacket);
  // wait for server response
  if (enet_host_service(client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_RECEIVE) {
    printResponseFromServer(event.packet);
  }

  while (true) {
    std::cin >> currentInput;
    // TODO should ping server (in separate thread) otherwise it will automatically disconnect after some time
    if (currentInput == "exit") {
      isForceDisconnect = true;
    } else if (currentInput == "help" || currentInput == "h") {
      showHelp();
    } else if (currentInput == "leaderboard" || currentInput == "lb") {
      sendPackageToServer(peer, "lb", currentPacket);
    } else if (currentInput.rfind("setusername:", 0) == 0) {
      username = currentInput.substr(12, currentInput.size());
      sendPackageToServer(peer, currentInput, currentPacket);
    } else if (isOnlyDigits(currentInput) && currentInput.size() > 0) {
      // guessing number
      sendPackageToServer(peer, "n:" + currentInput, currentPacket);
    } else {
      printf("Incorrect command. Print 'help' for more info.\n");
      continue;
    }

    if (isForceDisconnect) {
      break;
    }

    if (enet_host_service(client, &event, 1000) > 0) {
      switch (event.type) {
        case ENET_EVENT_TYPE_RECEIVE:
          printResponseFromServer(event.packet);
          break;
      }
    } else {
      printf("connection lost ...\n");
      // add new connection
      client = enet_host_create(NULL	/* the address to bind the server host to */,
                                1	/* allow up to 32 clients and/or outgoing connections */,
                                1	/* allow up to 1 channel to be used, 0. */,
                                0	/* assume any amount of incoming bandwidth */,
                                0	/* assume any amount of outgoing bandwidth */);
      if (client == NULL) {
        fprintf(stderr, "An error occurred while trying to create an ENet client host.\n");
        exit (EXIT_FAILURE);
      }
      peer = enet_host_connect(client, &address, 1, 0);
      if (peer == NULL) {
        fprintf(stderr, "No available peers for initiating an ENet connection!\n");
        return EXIT_FAILURE;
      }
      if (enet_host_service(client, &event, 2000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        printf("Connection to '%s:%i' succeeded.\n", addressAndPort.first.c_str(), addressAndPort.second);
      } else {
        enet_peer_reset(peer);
        printf("Connection to '%s:%i' failed.\n", addressAndPort.first.c_str(), addressAndPort.second);
        return EXIT_SUCCESS;
      }
      // update username in new connection
      setUsername(peer, "setusername:" + username, currentPacket);
      if (enet_host_service(client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_RECEIVE) {
        printResponseFromServer(event.packet);
      }
    }
  }

  // [...End game loop...]

  enet_peer_disconnect(peer, 0);

  while (enet_host_service(client, &event, 1500) > 0) {
    switch (event.type) {
      case ENET_EVENT_TYPE_RECEIVE:
        enet_packet_destroy(event.packet);
        break;
      case ENET_EVENT_TYPE_DISCONNECT:
        puts("Disconnection succeeded.");
        break;
    }
  }

  return EXIT_SUCCESS;
}
