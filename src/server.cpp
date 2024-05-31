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
#include <vector>
#include <zlib.h>
#include "../1.3.1/include/zlib.h"
#include <ios>
#include <iomanip>
#include <assert.h>

std::string getRequestBody(std::string &s, std::vector<std::string> &httpVect)
{
  std::ostringstream oss;
  int pos = 0;
  while ((pos = s.find("\r\n")) != std::string::npos)
  {
    pos = s.find("\r\n");
    httpVect.push_back(s.substr(0, pos));
    s.erase(0, pos + 2);
  }

  oss << s; // Add the remaining part of the string
  std::string result = oss.str();
  if (!result.empty() && result.back() == '\n')
  {
    result.pop_back(); // Remove the trailing newline character
  }
  return result;
}

std::string compress_string(const std::string &str)
{
  std::vector<char> buffer;
  z_stream zs;
  memset(&zs, 0, sizeof(zs));
  if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
  {
    throw(std::runtime_error("deflateInit2 failed"));
  }
  zs.next_in = (Bytef *)str.data();
  zs.avail_in = str.size();
  int ret;
  char outbuffer[32768];
  do
  {
    zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
    zs.avail_out = sizeof(outbuffer);
    ret = deflate(&zs, Z_FINISH);
    if (buffer.size() < zs.total_out)
    {
      buffer.insert(buffer.end(), outbuffer, outbuffer + zs.total_out - buffer.size());
    }
  } while (ret == Z_OK);
  deflateEnd(&zs);
  if (ret != Z_STREAM_END)
  {
    throw(std::runtime_error("deflate failed"));
  }
  return std::string(buffer.begin(), buffer.end());
}

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
      std::string httpRequest(recvBuf);

      // Endpoint checker
      bool listenForEcho = httpRequest.find("/echo/") != std::string::npos;
      bool listenForUserAgent = httpRequest.find("/user-agent") != std::string::npos;
      bool listenForFiles = httpRequest.find("/files/") != std::string::npos;

      // HTTP methods checker
      bool listenForPost = httpRequest.find("POST") != std::string::npos;

      // Send 404 on these conditions
      if ((recvBuf[5] != ' ') && (!listenForEcho) && (!listenForUserAgent) && (!listenForFiles))
      {
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        std::cout << "Client couldn't connect\n";
      }
      else if (listenForEcho)
      {

        bool listenForEncodingHeader = httpRequest.find("Accept-Encoding: ") != std::string::npos;
        if (listenForEncodingHeader)
        {

          std::string searchString = "/echo/";
          size_t startPos = httpRequest.find(searchString);
          startPos += searchString.length();
          size_t endPos = httpRequest.find(' ', startPos);
          std::string stringToBeCompressed = (endPos != std::string::npos) ? httpRequest.substr(startPos, endPos - startPos) : httpRequest.substr(startPos);
          int contentLength = stringToBeCompressed.length();

          std::string searchString1 = "Accept-Encoding: ";
          size_t startPos1 = httpRequest.find(searchString1);
          startPos1 += searchString1.length();
          size_t endPos1 = httpRequest.find("\r\n", startPos1);
          std::string responseBody1 = (endPos1 != std::string::npos) ? httpRequest.substr(startPos1, endPos1 - startPos1) : httpRequest.substr(startPos1);
          int contentLength1 = responseBody1.length();

          bool hasGzip = responseBody1.find("gzip") != std::string::npos;
          bool hasEncoding1 = responseBody1.find("encoding-1") != std::string::npos;
          bool hasEncoding2 = responseBody1.find("encoding-2") != std::string::npos;

          std::ostringstream oss;

          if (true)
          {
            std::string compressedString = compress_string(stringToBeCompressed);
            std::cout << "compressedString: " << compressedString << "\n";

            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Encoding: gzip\r\n"
                << "Content-Type: text/plain\r\n"
                << "Content-Length: " << (compressedString).length() << "\r\n\r\n"
                << compressedString;
          }
          else
          {
            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: text/plain\r\n"
                << "Content-Length: " << contentLength1 << "\r\n\r\n"
                << responseBody1;
          }

          std::string msgStr = oss.str();
          const char *msg = msgStr.c_str();
          send(clientSocket, msg, strlen(msg), 0);
          std::cout << "Client connected on /user-agent\n";
        }

        std::string searchString = "/echo/";
        size_t startPos = httpRequest.find(searchString);
        startPos += searchString.length();
        size_t endPos = httpRequest.find(' ', startPos);
        std::string responseBody = (endPos != std::string::npos) ? httpRequest.substr(startPos, endPos - startPos) : httpRequest.substr(startPos);
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
          size_t startPos = httpRequest.find(searchString);
          startPos += searchString.length();
          size_t endPos = httpRequest.find(' ', startPos);
          std::string fileName = (endPos != std::string::npos) ? httpRequest.substr(startPos, endPos - startPos) : httpRequest.substr(startPos);

          // get path of file
          // Ensure argv[2] ends with a slash
          std::string basePath = argv[2];

          if (basePath.back() != '/')
          {
            basePath += '/';
          }

          std::ifstream inputFile;
          inputFile.open(basePath + fileName);

          // if HTTP method is POST
          if (listenForPost)
          {
            std::ofstream outputFile;
            outputFile.open(basePath + fileName, std::ios::app);

            if (outputFile.is_open())
            {
              std::vector<std::string> httpVect;
              std::string fileContent = getRequestBody(httpRequest, httpVect);

              outputFile << fileContent;
              outputFile.close();

              std::ostringstream oss;
              oss << "HTTP/1.1 201 Created\r\n"
                  << "Content-Type: application/octet-stream\r\n"
                  << "Content-Length: " << std::to_string(fileContent.length()) << "\r\n\r\n"
                  << fileContent;
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
          }

          if (inputFile)
          {
            // get file's content size
            std::stringstream buffer;
            buffer << inputFile.rdbuf();
            std::string fileContent = buffer.str();

            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Content-Type: application/octet-stream\r\n"
                << "Content-Length: " << std::to_string(fileContent.length()) << "\r\n\r\n"
                << fileContent;
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
        size_t startPos = httpRequest.find(searchString);
        startPos += searchString.length();
        size_t endPos = httpRequest.find("\r\n", startPos);
        std::string responseBody = (endPos != std::string::npos) ? httpRequest.substr(startPos, endPos - startPos) : httpRequest.substr(startPos);
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
