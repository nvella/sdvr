#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

#define NAME "sdvr"
#define VERSION "0.0.1"

struct pk_s {
  int size;
  char *buffer;

  // in-buffer data

  int *d_magic;

  int *d_packet_id;
  int *d_payload_size;
  int *d_time;
  int *d_status;
};

struct pk_c_01_s {
  int size;
  char *buffer;

  // in-buffer data

  int *d_magic;

  int *d_packet_id;
  int *d_payload_size;
  int *d_time;

  char *d_username; // data username
  char *d_password; // data password
};

void error(char *msg) {
    perror(msg);
    exit(0);
}

void pk_s_init(struct pk_s *p) {
  // Setup offsets
  p->d_magic        = (int*)(p->buffer);
  p->d_packet_id    = (int*)(p->buffer + 4);
  p->d_payload_size = (int*)(p->buffer + 8);
  p->d_time         = (int*)(p->buffer + 12);
  p->d_status       = (int*)(p->buffer + 16);
  // Payload size and packet id still need to be set
}

void pk_s_destroy(void *pk_v) {
  struct pk_s *p = (struct pk_s *)pk_v;
  free(p->buffer);
}

void pk_c_01_s_new(struct pk_c_01_s *p) {
  bzero(p, sizeof(struct pk_c_01_s));

  p->size = 1812; // Packet is 1812 bytes in total;
  p->buffer = malloc(p->size);
  bzero(p->buffer, p->size); // Clear buffer

  // Setup offsets
  p->d_magic        = (int*)(p->buffer);
  p->d_packet_id    = (int*)(p->buffer + 4);
  p->d_payload_size = (int*)(p->buffer + 8);
  p->d_time         = (int*)(p->buffer + 12);
  p->d_username     = p->buffer + 20;
  p->d_password     = p->buffer + 52;

  // Set static values
  *(p->d_magic) = 0xABCDEF0; // Sent as little endian over network: F0 DE BC 0A
  *(p->d_packet_id) = 0x01; // Packet ID: 01 00 00 00
  *(p->d_payload_size) = p->size - 20; // or 1792
}

// Read a packet
struct pk_s *pk_s_read(int sock_fd) {
  struct pk_s *p = malloc(sizeof(struct pk_s));
  bzero(p, sizeof(struct pk_s));

  p->buffer = malloc(20); // Allocate enough space for header
  read(sock_fd, p->buffer, 20); // Read in bytes

  // Init pointers in packet
  pk_s_init(p);

  // Correctly set buffer size
  p->size = 20 + *(p->d_payload_size);

  // Check magic number
  if(*(p->d_magic) != 0xABCDEF0) error("magic number in packet not correct");

  // Reallocate packet for payload data
  p->buffer = realloc(p->buffer, 20 + *(p->d_payload_size));
  // Read in new data
  read(sock_fd, p->buffer + 20, *(p->d_payload_size));

  return p;
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
  free(auth_packet);

  // Read response packet
  struct pk_s *res_packet = pk_s_read(sock_fd);
  if(*(res_packet->d_packet_id) != 0x01) error("did not receive OK packet back");
  if(*(res_packet->d_status) != 200)     error("Authentication failed. (wrong username or password?)");

  // Authentication was successful
  fprintf(stderr, "Authenticated.\n");

  // close the socket
  close(sock_fd);
  return 0;
}
