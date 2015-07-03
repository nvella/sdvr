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

/*** Packet 03, Client to Server, Video control ***/
struct pk_c_03_s {
  int size;
  char *buffer;

  // in-buffer data

  int *d_magic;

  int *d_packet_id;
  int *d_payload_size;
  int *d_time;

  int *d_control; // Control variable, use currently unknown
};

/*** Packet 03, Server to Client, Video ***/
struct pk_s_03_s {
  int size;
  char *buffer;

  // in-buffer data

  int *d_magic;

  int *d_packet_id;
  int *d_payload_size;
  int *d_time;

  char *d_type_str; // type string, 4 characters
  char *d_video_type; // video type string, usually 4 characters/'H264'
  int *d_video_payload_size; // video payload size

  int *d_max_channels;
  int *d_channel_id;
  int *d_current_time;

  char *d_video_payload;
};

/*** Method declarations ***/

void pk_s_init(struct pk_s *);
void pk_s_destroy(void *);
struct pk_s *pk_s_read(int);

void pk_c_01_s_new(struct pk_c_01_s *);

void pk_s_01_s_init(struct pk_s_01_s *);

void pk_c_03_s_new(struct pk_c_03_s *);

void pk_s_03_s_init(struct pk_s_03_s *);
