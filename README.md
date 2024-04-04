# Shared Communication Library

Provide a cross-platform C++ library that can create shared communication instances for different applications.

* To run the example: 
1. Build the library
    ```bash
    mkdir build && cd build/
    cmake .. && cmake --build . --config Debug --target ALL_BUILD -j 18
    ```
2. Run ./build/Debug/test_server
3. Run ./build/Debug/test_client# multiple times up to 32.
4. You'll see the clients connect to the server with the same socket while the first client will own the socket. After it closes the connection, the socket will be transferred to the other clients.