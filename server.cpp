#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <bits/stdc++.h>

#include "enet/enet.h"



#define SERVER_VERSION "0.1.3"



void showHelp() {
  printf("\nUSAGE: server_app [server_port>]\n\n");
}

std::string convertUnetDataToString(enet_uint8 * data) {
  char* inputChars = (char *) data;
  std::string str = inputChars;
  return str;
}

double getAverageDifference(const std::pair<int, int> & attemptsAndTotalDifference) {
  return 1.0 * attemptsAndTotalDifference.second / attemptsAndTotalDifference.first;
}

std::string getLeaderboard(std::map<std::string, std::string> & mapSockerName,
                           const std::map<std::string, std::pair<int, int>> & mapNameStats) {
  std::string leaderboard = "\n";
  // vector of averageDifferenses for connected players
  std::vector<std::pair<std::string, double>> averageDifferenses;
  bool isUsernameConnected = false;

  for (auto itrNameStats = mapNameStats.begin(); itrNameStats != mapNameStats.end(); ++itrNameStats) {
    // search username in connected users
    isUsernameConnected = false;
    for (auto itrSocketName = mapSockerName.begin(); itrSocketName != mapSockerName.end(); ++itrSocketName) {
      if (itrSocketName->second == itrNameStats->first) {
        isUsernameConnected = true;
      }
    }
    // check username connected and already played some games
    if (isUsernameConnected && itrNameStats->second.first != NULL) {
      averageDifferenses.push_back({ itrNameStats->first, getAverageDifference(itrNameStats->second) });
    }
  }
  std::sort(averageDifferenses.begin(), 
            averageDifferenses.end(), 
            [](std::pair<std::string, double> a, std::pair<std::string, double> b) { return a.second < b.second; });
  int i=0;
  for (auto itr = averageDifferenses.begin(); itr != averageDifferenses.end(); ++itr) {
    leaderboard += std::to_string(i + 1) + ") " + itr->first + ": " + std::to_string(itr->second) + "\n";
    i++;
  }
  return leaderboard;
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
          currentInput = convertUnetDataToString(event.packet->data);
          response = "-\n";
          if (currentInput == "lb") {
            response = "Leaderboard:\n";
            response += getLeaderboard(mapSockerName, mapNameStats);
          } else if (currentInput.rfind("setusername:", 0) == 0) {
            std::string newUsername = currentInput.substr(12, currentInput.size());
            if (mapNameStats.find(mapSockerName[currentUserSocket]) == mapNameStats.end()) {
              mapNameStats[newUsername] = mapNameStats[mapSockerName[currentUserSocket]];
            }
            mapNameStats.erase(mapSockerName[currentUserSocket]);
            mapSockerName[currentUserSocket] = newUsername;
            response = "New username: " + newUsername + "\n";
          } else if (currentInput.rfind("n:", 0) == 0) {
            guessedNumber = rand() % 101;  // [0, 100]
            currentUserNumber = std::stoi(currentInput.substr(2, currentInput.size()));
            currentUserDifference = std::abs(guessedNumber - currentUserNumber);
            mapNameStats[mapSockerName[currentUserSocket]].first += 1;
            mapNameStats[mapSockerName[currentUserSocket]].second += currentUserDifference;
            response = "Guessed number was: " + std::to_string(guessedNumber) + "\n";
            response += "Your difference: " + std::to_string(currentUserDifference) + "\n";
            response += "Average difference: " + std::to_string(getAverageDifference(mapNameStats[mapSockerName[currentUserSocket]])) + "\n";
          } else {
            response = "Bad request.\n";
          }
          /* Clean up the packet now that we're done using it. */
          enet_packet_destroy(event.packet);
          sendResponse(event.peer, response, responsePacket);
          break;
        case ENET_EVENT_TYPE_DISCONNECT:
          printf("%s disconnected.\n", mapSockerName[currentUserSocket].c_str());
          mapSockerName.erase(currentUserSocket);
          /* Reset the peer's client information. */
          event.peer->data = NULL;
          break;
      }
    }
  }

  enet_host_destroy(server);

  return 0;
}