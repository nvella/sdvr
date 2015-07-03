#pragma once

/*** Generic packet ***/
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

/*** Packet 01, Client to Server, Login ***/
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

/*** Packet 01, Server to Client, Login Response ***/
struct pk_s_01_s {
  int size;
  char *buffer;

  // in-buffer data

  int *d_magic;

  int *d_packet_id;
  int *d_payload_size;
  int *d_time;

  char *d_name; // DVR Name
  char *d_vdef; // DVR Video definition (960H)
};

/*** Method declarations ***/

void pk_s_init(struct pk_s *);
void pk_s_destroy(void *);
struct pk_s *pk_s_read(int);

void pk_c_01_s_new(struct pk_c_01_s *);

void pk_s_01_s_init(struct pk_s_01_s *);
