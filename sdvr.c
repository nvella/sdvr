#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

#include "sdvr.h"
#include "pk.h"

char dvr_name[32];
char dvr_vdef[32];

void error(char *msg) {
    perror(msg);
    exit(0);
}

// This function only exists because read() sometimes returns less data than it
// should, even though there is more data in the socket.
size_t stable_read(int fd, void *buffer, size_t count) {
  size_t read_so_far = 0;
  while(read_so_far < count) {
    int read_this_go = read(fd, buffer + read_so_far, count - read_so_far);
    if(read_this_go < 0) return read_this_go;
    read_so_far += read_this_go;
  }
  return read_so_far;
}

int main(int argc, char** argv) {
  if(argc < 5) {
    printf("usage: sdvr ip port username password\nport is usually 9000\n");
    return 1;
  }

  fprintf(stderr, "%s version %s\n", NAME, VERSION);

  struct hostent *dvr_host = gethostbyname(argv[1]);
  if(dvr_host == NULL) error("no such host");
  int dvr_port = atoi(argv[2]);

  struct sockaddr_in dvr;
  bzero(&dvr, sizeof(struct sockaddr_in));
  dvr.sin_family = AF_INET;
  // Copy the address
  memcpy(&(dvr.sin_addr.s_addr), dvr_host->h_addr, dvr_host->h_length);
  // Copy the port
  dvr.sin_port = htons(dvr_port);

  // Create the socket
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(sock_fd < 0) error("failed to open socket");
  // Connect the socket
  if(connect(sock_fd, &dvr, sizeof(struct sockaddr_in)) < 0) error("failed to connect");
  fprintf(stderr, "Socket connected.\n");

  // Socket connected at this point, now;
  // 1. Send auth packet
  // 2. Receive initial response
  // 3. (maybe send other packets)
  // 4. Receive frames

  // Create auth packet
  struct pk_c_01_s *auth_packet = malloc(sizeof(struct pk_c_01_s));
  pk_c_01_s_new(auth_packet); // Init packet
  *(auth_packet->d_time) = time(NULL); // Set unix time in packet
  strcpy(auth_packet->d_username, argv[3]); // Copy in username
  strcpy(auth_packet->d_password, argv[4]); // Copy in password

  // Send auth packet
  if(write(sock_fd, auth_packet->buffer, auth_packet->size) < 0)
    error("couldn't send auth packet");

  // Destroy auth packet
  pk_s_destroy(auth_packet);

  // Read response packet
  struct pk_s *res_packet = pk_s_read(sock_fd);
  if(*(res_packet->d_packet_id) != 0x01) error("did not receive OK packet back");
  if(*(res_packet->d_status) != 200)     error("Authentication failed. (wrong username or password?)");

  // Create Packet 0x1 S->C structure and copy data in
  struct pk_s_01_s pk_01_res;
  memcpy(&pk_01_res, res_packet, sizeof(struct pk_s));
  free(res_packet); // Only freeing the packet struct to keep the buffer
  pk_s_01_s_init(&pk_01_res); // Init the pointers
  strcpy(dvr_name, pk_01_res.d_name); // Copy the DVR name to the global
  strcpy(dvr_vdef, pk_01_res.d_vdef); // Copy the DVR vdef to the global
  free(pk_01_res.buffer); // Free the packet buffer (cant free the struct as it's defined on the stack)

  // Print some status data
  fprintf(stderr, "DVR Name: %s\nVideo definition: %s\n", dvr_name, dvr_vdef);

  // Authentication was successful
  fprintf(stderr, "Authenticated.\n");

  // Enter a loop, receive packets until a 0x99 packet is received
  int packet_id = -1;
  while(packet_id != 0x99) {
    struct pk_s *pk = pk_s_read(sock_fd);
    // Print the packet ID
    fprintf(stderr, "Received packet: 0x%02x\n", *(pk->d_packet_id));
    packet_id = *(pk->d_packet_id);
    // Free the packet, avoid memory leaks
    pk_s_destroy(pk);
  }

  // close the socket
  close(sock_fd);
  return 0;
}
