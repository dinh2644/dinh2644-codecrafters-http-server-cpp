#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char **argv)
{
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  //
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }
  //
  // // Since the tester restarts your program quite often, setting REUSE_PORT
  // // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  // Accept call is blocking until a client connects to the server via server_fd
  while (true)
  {
    int client = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);

    const char *successMsg = "HTTP/1.1 200 OK\r\n\r\n";
    const char *errorMsg = "HTTP/1.1 404 Not Found\r\n\r\n";

    // Get URL's content
    char recvBuf[512];
    int urlLength = recv(client, recvBuf, sizeof(recvBuf), 0);
    std::string requestURL(recvBuf);

    // ENDPOINT CHECKER
    bool listenForEcho = requestURL.find("/echo/") != std::string::npos;
    bool listenForUserAgent = requestURL.find("/user-agent") != std::string::npos;

    // Send 404 on these conditions
    if ((recvBuf[5] != ' ') && (!listenForEcho) && (!listenForUserAgent))
    {
      send(client, errorMsg, strlen(errorMsg), 0);
      std::cout << "Client couldn't connect\n";
      close(server_fd);
      return 1;
    }

    if (listenForEcho)
    {
      std::string searchString = "/echo/";
      size_t startPos = requestURL.find(searchString);
      startPos += searchString.length();
      size_t endPos = requestURL.find(' ', startPos);

      std::string responseBody = (endPos != std::string::npos) ? requestURL.substr(startPos, endPos - startPos) : requestURL.substr(startPos);
      int contentLength = responseBody.length();

      std::ostringstream oss;
      oss << "HTTP/1.1 200 OK\r\n"
          << "Content-Type: text/plain\r\n"
          << "Content-Length: " << contentLength << "\r\n\r\n"
          << responseBody;

      std::string msgStr = oss.str();
      const char *msg = msgStr.c_str();

      send(client, msg, strlen(msg), 0);
      std::cout << "Client connected\n";
      close(server_fd);
      return 1;
    }

    if (listenForUserAgent)
    {
      std::string searchString = "User-Agent: ";
      size_t startPos = requestURL.find(searchString);
      startPos += searchString.length();
      size_t endPos = requestURL.find("\r\n", startPos);

      std::string responseBody = (endPos != std::string::npos) ? requestURL.substr(startPos, endPos - startPos) : requestURL.substr(startPos);
      int contentLength = responseBody.length();

      std::ostringstream oss;
      oss << "HTTP/1.1 200 OK\r\n"
          << "Content-Type: text/plain\r\n"
          << "Content-Length: " << contentLength << "\r\n\r\n"
          << responseBody;

      std::string msgStr = oss.str();
      const char *msg = msgStr.c_str();

      send(client, msg, strlen(msg), 0);
      std::cout << "Client connected\n";
      close(server_fd);
      return 1;
    }

    int bytesSent = 0;
    int totalBytesSent = 0;
    while (bytesSent < totalBytesSent)
    {
      bytesSent = send(client, successMsg, strlen(successMsg), 0);
      if (bytesSent < 0)
      {
        std::cout << "Could not send response" << "\n";
      }
      totalBytesSent += bytesSent;
    }

    std::cout << "Client connected\n";
    close(client);
  }

  close(server_fd);

  return 0;
}
