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
#include <fstream>

int main(int argc, char **argv)
{
  // CREATE SOCKET //
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }
  // SET SOCKET OPTIONS //
  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
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
  // BIND SOCKET TO PORT 4221 //
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  // MAKE SOCKET LISTEN (put connections in a queue of connection_backlog capacity) //
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  }
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  std::cout << "Waiting for a client to connect...\n";

  // Child process id

  // Client socket
  int clientSocket;
  while (true)
  {
    const char *successMsg = "HTTP/1.1 200 OK\r\n\r\n";
    const char *errorMsg = "HTTP/1.1 404 Not Found\r\n\r\n";
    // ACCEPT ANY INCOMING CONNECTIONS //
    // Accept call is blocking until a client connects to the server via server_fd
    // Creates new socket with a connection thread
    clientSocket = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    // Error handling
    if (clientSocket < 0)
    {
      exit(1);
    }
    pid_t childpid = fork();

    // Concurrent Server Approach: call fork() each time a new client connects to server:
    if (childpid == 0)
    {
      close(server_fd); // Close the listening socket in the child process

      // Get URL's content
      char recvBuf[512];
      int urlLength = recv(clientSocket, recvBuf, sizeof(recvBuf), 0);
      std::string requestURL(recvBuf);

      // Endpoint checker
      bool listenForEcho = requestURL.find("/echo/") != std::string::npos;
      bool listenForUserAgent = requestURL.find("/user-agent") != std::string::npos;
      bool listenForFiles = requestURL.find("/files/") != std::string::npos;

      // Send 404 on these conditions
      if ((recvBuf[5] != ' ') && (!listenForEcho) && (!listenForUserAgent) && (!listenForFiles))
      {
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        std::cout << "Client couldn't connect\n";
      }
      else if (listenForEcho)
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
        send(clientSocket, msg, strlen(msg), 0);
        std::cout << "Client connected on /echo\n";
      }
      else if (listenForFiles)
      {
        if (argc > 2)
        {
          std::string searchString = "/files/";
          size_t startPos = requestURL.find(searchString);
          startPos += searchString.length();
          size_t endPos = requestURL.find(' ', startPos);
          std::string responseBody = (endPos != std::string::npos) ? requestURL.substr(startPos, endPos - startPos) : requestURL.substr(startPos);

          // get path of file
          // Ensure argv[2] ends with a slash
          std::string basePath = argv[2];
          if (basePath.back() != '/')
          {
            basePath += '/';
          }

          std::string fileToCheck = basePath + responseBody;

          std::ifstream inputFile(fileToCheck);

          if (inputFile.is_open())
          {
            // get file's content length
            int fileSize = inputFile.tellg();

            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: application/octet-stream\r\n"
                << "Content-Length: " << fileSize << "\r\n\r\n"
                << responseBody;
            std::string msgStr = oss.str();
            const char *msg = msgStr.c_str();
            send(clientSocket, msg, strlen(msg), 0);
            std::cout << "Client connected on /files\n";
          }
          else
          {
            // return 404
            send(clientSocket, errorMsg, strlen(errorMsg), 0);
            std::cout << "File doesn't exist\n";
          }
          close(clientSocket);
          exit(0);
        }
        else
        {
          std::cout << "Directory path not provided" << std::endl;
          close(clientSocket);
          exit(0);
        }
      }
      else if (listenForUserAgent)
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
        send(clientSocket, msg, strlen(msg), 0);
        std::cout << "Client connected on /user-agent\n";
      }
      else
      {
        // If no endpoint, send normal 202 response
        send(clientSocket, successMsg, strlen(successMsg), 0);
        std::cout << "Client connected without endpoint\n";
      }

      close(clientSocket);
      exit(0);
    }
    else
    {
      close(clientSocket); // Close the connected socket in the parent process
    }
  }

  close(server_fd);
  std::cout << "All connections done \n"
            << std::endl;

  return 0;
}
