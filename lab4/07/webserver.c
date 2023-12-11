#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define RESPONSE_TEMPLATE \
  "HTTP/1.1 200 OK\nContent-Length: %zu\nContent-Type: text/html\n\n%s"

void handle_get_request(int client_socket, const char* request_path) {
  char response_buffer[BUFFER_SIZE];
  char file_path[BUFFER_SIZE];
  FILE* file;

  if (strcmp(request_path, "/") == 0) {
    strcpy(file_path, "index.html");
  } else {
    sprintf(file_path, "%s", request_path + 1);
  }

  if ((file = fopen(file_path, "r")) == NULL) {
    snprintf(response_buffer, BUFFER_SIZE,
             "HTTP/1.1 404 Not Found\nContent-Length: 13\n\nFile Not Found");
  } else {
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* file_content = (char*)malloc(file_size + 1);
    fread(file_content, 1, file_size, file);
    fclose(file);
    file_content[file_size] = '\0';

    snprintf(response_buffer, BUFFER_SIZE, RESPONSE_TEMPLATE, file_size,
             file_content);

    free(file_content);
  }

  send(client_socket, response_buffer, strlen(response_buffer), 0);
}

void handle_post_request(int client_socket,
                         const char* request_path,
                         const char* post_data) {
  char response_buffer[BUFFER_SIZE];
  snprintf(response_buffer, BUFFER_SIZE, RESPONSE_TEMPLATE, strlen(post_data),
           post_data);
  send(client_socket, response_buffer, strlen(response_buffer), 0);
}

int main() {
  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  char buffer[BUFFER_SIZE];

  if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_socket, (struct sockaddr*)&server_addr,
           sizeof(server_addr)) == -1) {
    perror("Binding failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_socket, 5) == -1) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  printf("Web server listening on port %d...\n", PORT);

  while (1) {
    if ((client_socket = accept(server_socket, (struct sockaddr*)&client_addr,
                                &client_addr_len)) == -1) {
      perror("Accept failed");
      exit(EXIT_FAILURE);
    }

    printf("Client connected\n");

    memset(buffer, 0, sizeof(buffer));
    if (recv(client_socket, buffer, sizeof(buffer), 0) == -1) {
      perror("Receive failed");
      exit(EXIT_FAILURE);
    }

    char method[10];
    char path[255];
    sscanf(buffer, "%s %s", method, path);

    if (strcmp(method, "POST") == 0) {
      char* content_length_start = strstr(buffer, "Content-Length:");
      if (content_length_start != NULL) {
        int content_length;
        sscanf(content_length_start, "Content-Length: %d", &content_length);

        char post_data[BUFFER_SIZE];
        int total_read = 0;

        while (total_read < content_length) {
          int bytes_read = recv(client_socket, post_data + total_read,
                                content_length - total_read, 0);
          if (bytes_read == -1) {
            perror("Receive failed");
            exit(EXIT_FAILURE);
          }
          total_read += bytes_read;
        }

        post_data[total_read] = '\0';
        handle_post_request(client_socket, path, post_data);
      }
    } else if (strcmp(method, "GET") == 0) {
      handle_get_request(client_socket, path);
    }

    close(client_socket);
    printf("Client disconnected\n");
  }
  close(server_socket);

  return 0;
}
