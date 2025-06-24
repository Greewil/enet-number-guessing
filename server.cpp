#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <bits/stdc++.h>

#include "enet/enet.h"



#define SERVER_VERSION "0.1.2"



void showHelp() {
  printf("\nUSAGE: server_app [server_port>]\n\n");
}

std::string convertUnetDataToString(enet_uint8 * data) {
  char* inputChars = (char *) data;
  std::string str = inputChars;
  return str;
}

std::string getLeaderboard(std::map<std::string, std::string> & mapSockerName,
                           const std::map<std::string, std::pair<int, int>> & mapNameStats) {
  std::string leaderboard = "";
  // vector of averageDifferenses for connected players
  std::vector<std::pair<std::string, double>> averageDifferenses;

  for (auto itr = mapNameStats.begin(); itr != mapNameStats.end(); ++itr) {
    // TODO select only namef from mapSockerName
    // name itr->first 
    // pair<int, int> stats it->second.first
  }
  //TODO sort vec
  // append all elements to leaderboard
}

void sendResponse(ENetPeer* peer, const std::string & responseData, ENetPacket * packet) {
  packet = enet_packet_create(responseData.c_str(),
                              responseData.size() + 1,
                              ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}



int main (int argc, char** argv) {
  std::srand(std::time({}));

  if (enet_initialize() != 0) {
    fprintf(stderr, "An error occurred while initializing ENet.\n");
    return EXIT_FAILURE;
  }
  atexit(enet_deinitialize);

  showHelp();

  int serverPort = 7777;
  if (argv[1] != NULL) {
    serverPort = std::stoi(argv[1]);
  }

  printf("Starting server (version %s)\n", SERVER_VERSION);
  // TODO check versions compatable while clients connecting to server

  ENetEvent event;
  ENetAddress address;
  ENetHost* server;

  /* Bind the server to the default localhost.     */
  /* A specific host address can be specified by   */
  /* enet_address_set_host (& address, "x.x.x.x"); */
  address.host = ENET_HOST_ANY; // This allows to start on current host
  /* Bind the server to port serverPort. */
  address.port = serverPort;

  server = enet_host_create(&address	/* the address to bind the server host to */,
                            64	/* allow up to 32 clients and/or outgoing connections */,
                            1	/* allow up to 1 channel to be used, 0. */,
                            0	/* assume any amount of incoming bandwidth */,
                            0	/* assume any amount of outgoing bandwidth */);

  if (server == NULL) {
    printf("An error occurred while trying to create an ENet server host.");
    return 1;
  }

  printf("Server started and listening port %i\n", address.port);

  // <userSocket, username>
  std::map<std::string, std::string> mapSockerName = {};
  // <username, pair<attempts, totalDifference>>
  std::map<std::string, std::pair<int, int>> mapNameStats = {};

  int guessedNumber = 42;
  std::string currentUserSocket = "";
  int currentUserNumber = 0;
  int currentUserDifference = 0;
  std::string currentInput = "";
  std::string response = "";
  while (true) {
    ENetEvent event;
    ENetPacket* responsePacket;
    /* Wait up to 1000 milliseconds for an event. */
    while (enet_host_service(server, &event, 1000) > 0) {
      currentUserSocket = std::to_string((int) event.peer->address.host) + ":" + std::to_string((int) event.peer->address.port);
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
          printf("A new client connected from %x:%u.\n",
                 event.peer->address.host,
                 event.peer->address.port);
          if (mapSockerName.find(currentUserSocket) == mapSockerName.end()) {
            mapSockerName.insert({currentUserSocket, std::to_string(rand())});
            printf("Added new user: (username = %s, socket = %s).\n", mapSockerName[currentUserSocket].c_str(), currentUserSocket.c_str());
            mapNameStats[mapSockerName[currentUserSocket]] = {0, 0.0};
          }
          break;
        case ENET_EVENT_TYPE_RECEIVE:
          printf("A packet of length %u containing '%s' was received from %x:%u on channel %u.\n",
                 event.packet->dataLength,
                 event.packet->data,
                 event.peer->address.host,
                 event.peer->address.port,
                 event.channelID);
          // TODO store number of guesses and deviation summ for each user
          // store it in map using sockets as keys: "event.peer->address.host : event.peer->address.port"
          // TODO count average deviation for current user using event.packet
          /* Clean up the packet now that we're done using it. */
          currentInput = convertUnetDataToString(event.packet->data);
          response = "-\n";
          if (currentInput == "lb") {
            response = "Leaderboard:\n";
            response += "TODO print leaderboard\n";
          } else if (currentInput.rfind("setusername:", 0) == 0) {
            std::string newUsername = currentInput.substr(12, currentInput.size());
            mapNameStats[newUsername] = mapNameStats[mapSockerName[currentUserSocket]];
            mapNameStats.erase(mapSockerName[currentUserSocket]);
            mapSockerName[currentUserSocket] = newUsername;
            response = "New username: " + newUsername + "\n";
          } else if (currentInput.rfind("n:", 0) == 0) {
            currentUserNumber = std::stoi(currentInput.substr(2, currentInput.size()));
            currentUserDifference = std::abs(guessedNumber - currentUserNumber);
            mapNameStats[mapSockerName[currentUserSocket]].first += 1;
            mapNameStats[mapSockerName[currentUserSocket]].second += currentUserDifference;
            guessedNumber = rand() % (101);  // [0, 100]
            response = "Guessed number was: " + std::to_string(guessedNumber) + "\n";
            response += "Your difference: " + std::to_string(currentUserDifference) + "\n";
            response += "Average difference: " + std::to_string(1.0 * mapNameStats[mapSockerName[currentUserSocket]].second / mapNameStats[mapSockerName[currentUserSocket]].first) + "\n";
          } else {
            response = "Bad request.\n";
          }
          enet_packet_destroy(event.packet);
          sendResponse(event.peer, response, responsePacket);
          break;
        case ENET_EVENT_TYPE_DISCONNECT:
          // TODO remove user from socketName map
          printf("%s disconnected.\n", event.peer->data);
          /* Reset the peer's client information. */
          event.peer->data = NULL;
          break;
      }
    }
  }

  enet_host_destroy(server);

  return 0;
}