#include <iostream>
#include <cstring>
#include <bits/stdc++.h>

#include "enet/enet.h"



#define CLIENT_VERSION "1.0.0"



std::string convertUnetDataToString(enet_uint8 * data) {
  char* inputChars = (char *) data;
  std::string str = inputChars;
  return str;
}

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

void sendPackageToServer(ENetPeer * peer, ENetPacket * packet, const std::string & sendingData) {
  /* Create a reliable packet of size sendingData.size() + 1 containing "${sendingData}\0" */
  packet = enet_packet_create(sendingData.c_str(),
                              sendingData.size() + 1,
                              ENET_PACKET_FLAG_RELIABLE);
  /* Send the packet to the peer over channel id 0. */
  enet_peer_send(peer, 0, packet);
}

void printResponseFromServer(ENetPacket * packet) {
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

void setUsername(ENetPeer * peer, ENetPacket * packet, const std::string & newUsername) {
  sendPackageToServer(peer, packet, "setusername:" + newUsername);
}

// void checkVersionsCompatible(ENetPeer * peer, ENetPacket * packet) {
//   ENetEvent event;
//   sendPackageToServer(peer, currentPacket, "version");
//   if (enet_host_service(client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_RECEIVE) {
//     printf("Server version: %s.\n", event.packet->data);
//     if (convertUnetDataToString(event.packet->data) != CLIENT_VERSION) {
//       exit(EXIT_FAILURE);
//     }
//   }
// }

std::pair<ENetHost*, ENetPeer*> connectToServer(const std::pair<std::string, int> & addressAndPort) {
  ENetHost* client;
  ENetPeer* peer;

  client = enet_host_create(NULL	/* the address to bind the server host to */,
                            1	/* allow up to 32 clients and/or outgoing connections */,
                            1	/* allow up to 1 channel to be used, 0. */,
                            0	/* assume any amount of incoming bandwidth */,
                            0	/* assume any amount of outgoing bandwidth */);
  if (client == NULL) {
    fprintf(stderr, "An error occurred while trying to create an ENet client host.\n");
    exit(EXIT_FAILURE);
  }
  
  ENetAddress address;
  enet_address_set_host(&address, addressAndPort.first.c_str());
  address.port = addressAndPort.second;

  peer = enet_host_connect(client, &address, 1, 0);
  if (peer == NULL) {
    fprintf(stderr, "No available peers for initiating an ENet connection!\n");
    exit(EXIT_FAILURE);
  }

  ENetEvent event;
  if (enet_host_service(client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
    printf("Connection to '%s:%i' succeeded.\n", addressAndPort.first.c_str(), addressAndPort.second);
  } else {
    enet_peer_reset(peer);
    printf("Connection to '%s:%i' failed.\n", addressAndPort.first.c_str(), addressAndPort.second);
    exit(EXIT_FAILURE);
  }
  
  return {client, peer};
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
  std::string username = "tumba-yumba-" + std::to_string(randNum);
  if (argv[1] != NULL && argv[2] != NULL) {
    username = argv[2];
  }

  printf("Starting client (version %s)\n", CLIENT_VERSION);

  std::pair<ENetHost*, ENetPeer*> clientPeer = connectToServer(addressAndPort);
  ENetHost* client = clientPeer.first;
  ENetPeer* peer = clientPeer.second;
  ENetEvent event;
  ENetPacket* currentPacket;

  // check server have compatible version (same major version parts, otherwise disconnect)
  sendPackageToServer(peer, currentPacket, "version");
  if (enet_host_service(client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_RECEIVE) {
    printf("Server version: %s\n", event.packet->data);
    std::string clientVersion = CLIENT_VERSION;
    std::string clientMajorVersion = clientVersion.substr(0, 2);
    // std::string clientMajorVersion = clientVersion.substr(0, clientVersion.rfind(".", 0));
    std::string serverVersion = convertUnetDataToString(event.packet->data);
    std::string serverMajorVersion = serverVersion.substr(0, 2);
    // std::string serverMajorVersion = serverVersion.substr(0, serverVersion.find(".", 0));
    // std::cout << clientMajorVersion << std::endl;
    // std::cout << serverMajorVersion << std::endl;
    if (clientMajorVersion != serverMajorVersion) {
      printf("Incompatibale major version!\n");
      exit(EXIT_FAILURE);
    }
  }

  // Sending current username
  // (so server will store this username in map and it will be associated with current client's socket)
  // later this username will be used in leaderboard
  setUsername(peer, currentPacket, username);
  // wait for server response
  if (enet_host_service(client, &event, 1000) > 0 && event.type == ENET_EVENT_TYPE_RECEIVE) {
    printResponseFromServer(event.packet);
  }

  // [...Game Loop...]

  std::string currentInput = "";
  bool isForceDisconnect = false;

  while (true) {
    std::cin >> currentInput;
    // TODO should ping server (in separate thread) otherwise it will automatically disconnect after some time
    if (currentInput == "exit") {
      isForceDisconnect = true;
    } else if (currentInput == "help" || currentInput == "h") {
      showHelp();
      continue;
    } else if (currentInput == "leaderboard" || currentInput == "lb") {
      sendPackageToServer(peer, currentPacket, "lb");
    } else if (currentInput.rfind("setusername:", 0) == 0) {
      username = currentInput.substr(12, currentInput.size());
      sendPackageToServer(peer, currentPacket, currentInput);
    } else if (isOnlyDigits(currentInput) && currentInput.size() > 0) {
      // guessing number
      sendPackageToServer(peer, currentPacket, "n:" + currentInput);
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
      std::pair<ENetHost*, ENetPeer*> clientPeer = connectToServer(addressAndPort);
      client = clientPeer.first;
      peer = clientPeer.second;
      // update username in new connection
      setUsername(peer, currentPacket, username);
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
