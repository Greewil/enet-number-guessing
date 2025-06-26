# enet-number-guessing
Simple client-server number guessing game based on enet library.

## Building

To generate sources with Makefile in build dir:
```shell
cmake -DCMAKE_BUILD_TYPE=MinSizeRel -B build
```

To build all apps from sources with CMake:
```shell
cmake --build build
```

To build only client app:
```shell
cmake --build build -t client_number_guessing_enet
```

To build only server app:
```shell
cmake --build build -t server_number_guessing_enet
```

# Usage

Client and server should have same major versions (fe. 1.2.3 and 1.43.52).

To run server app:
```shell
# server_app PORT
./bin/Linux64/MinSizeRel/server_number_guessing_enet 7777
```

To run client app:
```shell
# client_app ADDRESS:PORT USERNAME
./bin/Linux64/MinSizeRel/server_number_guessing_enet 127.0.0.1:7777 username
```
