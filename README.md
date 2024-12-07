# Make setup script executable
```sh
chmod +x setup.sh
```

# Run setup script
```sh
./setup.sh
```

# Build project
```sh
cmake -B build
cmake --build build
```

# Run the application
```sh
./build/deribit_trader
```


# Project Struture

```sh
├── CMakeLists.txt
├── include
│   ├── market_data
│   │   ├── market_data_handler.h
│   │   └── orderbook.h
│   ├── order_management
│   │   ├── order.h
│   │   └── order_manager.h
│   ├── performance
│   │   └── latency_tracker.h
│   ├── utils
│   │   ├── config_loader.h
│   │   ├── config_manager.h │
│   │   └── logger.h
│   └── websocket
│       ├── websocket_client.h
│       └── websocket_server.h
├── README.md
├── setup.sh
├── src
│   ├── main.cpp
│   ├── market_data
│   │   ├── market_data_handler.cpp
│   │   └── orderbook.cpp
│   ├── order_management
│   │   ├── order.cpp
│   │   └── order_manager.cpp
│   ├── performance
│   │   └── latency_tracker.cpp
│   ├── utils
│   │   ├── config_manager.cpp
│   │   └── logger.cpp
│   └── websocket
│       ├── websocket_client.cpp
│       └── websocket_server.cpp
└── tests
    ├── order_management_tests.cpp
    ├── performance_tests.cpp
    └── websocket_tests.cpp
```