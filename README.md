# enet-number-guessing
Simple client-server number guessing game based on enet library.

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

To run server app:
```shell
./bin/Linux64/MinSizeRel/server_number_guessing_enet
```
