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
  free(p);
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

void pk_s_01_s_init(struct pk_s_01_s *p) {
  // Setup offsets
  p->d_magic        = (int*)(p->buffer);
  p->d_packet_id    = (int*)(p->buffer + 4);
  p->d_payload_size = (int*)(p->buffer + 8);
  p->d_time         = (int*)(p->buffer + 12);

  p->d_name         = p->buffer + 80;
  p->d_vdef         = p->buffer + 160;
}

// Read a packet
struct pk_s *pk_s_read(int sock_fd) {
  struct pk_s *p = malloc(sizeof(struct pk_s));
  bzero(p, sizeof(struct pk_s));

  p->buffer = malloc(20); // Allocate enough space for header

  // Error if memory not available
  if(p->buffer == NULL || p == NULL) error("Out of memory");

  // read in bytes
  if(stable_read(sock_fd, p->buffer, 20) != 20) error("Failed to read packet header");

  // Init pointers in packet
  pk_s_init(p);

  // Correctly set buffer size
  p->size = 20 + *(p->d_payload_size);

  // Check magic number
  if(*(p->d_magic) != 0xABCDEF0) {
    fprintf(stderr, "magic number: %08x\n", *(p->d_magic));
    error("magic number in packet not correct");
  }

  // Reallocate packet for payload data
  p->buffer = realloc(p->buffer, 20 + *(p->d_payload_size));
  // Read in new data
  if(stable_read(sock_fd, p->buffer + 20, *(p->d_payload_size)) != *(p->d_payload_size)) {
    fprintf(stderr, "payload expected: %i\n", *(p->d_payload_size));
    error("Failed to read packet payload");
  }

  return p;
}

void pk_c_03_s_new(struct pk_c_03_s *p) {
  bzero(p, sizeof(struct pk_c_03_s));

  p->size = 32; // Packet is 32 bytes in total;
  p->buffer = malloc(p->size);
  bzero(p->buffer, p->size); // Clear buffer

  // Setup offsets
  p->d_magic        = (int*)(p->buffer);
  p->d_packet_id    = (int*)(p->buffer + 4);
  p->d_payload_size = (int*)(p->buffer + 8);
  p->d_control      = (int*)(p->buffer + 28);

  // Set static values
  *(p->d_magic) = 0xABCDEF0; // Sent as little endian over network: F0 DE BC 0A
  *(p->d_packet_id) = 0x03; // Packet ID: 01 00 00 00
  *(p->d_payload_size) = p->size - 20; // or 12
  *(p->d_control) = 0x01; // no one knows why but it works
}

void pk_s_03_s_init(struct pk_s_03_s *p) {
  // Setup offsets
  p->d_magic        = (int*)(p->buffer);
  p->d_packet_id    = (int*)(p->buffer + 4);
  p->d_payload_size = (int*)(p->buffer + 8);
  p->d_time         = (int*)(p->buffer + 12);

  p->d_type_str           = p->buffer + 20;
  p->d_video_type         = p->buffer + 24;
  p->d_video_payload_size = (int*)(p->buffer + 28);

  p->d_max_channels       = (int*)(p->buffer + 32);
  p->d_channel_id         = (int*)(p->buffer + 40);
  p->d_current_time       = (int*)(p->buffer + 44);

  p->d_video_payload      = p->buffer + 52;
}
