A simple HTTP server, the protocol that powers the web.

## Features

- **TCP Connections**: Manage TCP connections.
- **HTTP Headers/Body**: Parse and interpret HTTP headers/body.
- **HTTP Request Methods**: Handle HTTP methods like GET and POST.
- **File Serving**: Send GET/POST file requests to server.
- **Concurrent Connections**: Manage multiple concurrent connections (via child processes).

## Getting Started

1. **Clone the Repository**:
    ```sh
    git clone https://github.com/dinh2644/http-server.git
    cd http-server
    ```

2. **Run the Server in /src**:
    ```sh
    g++ server.cpp -o server
    ./server
    ```

3. **Start Sending Requests**:
    - Use a web browser or a tool like `curl` to send GET/POST requests.
    - Access files served by the server.
    ```sh
    // Example GET request
    curl -i -X GET http://localhost:4221/index.html
    ```

## Project Structure

- `/src/server.cpp`: The main file where the HTTP server implementation resides.
