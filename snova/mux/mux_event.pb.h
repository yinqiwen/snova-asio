/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.7-dev */

#ifndef PB_SNOVA_MUX_EVENT_PB_H_INCLUDED
#define PB_SNOVA_MUX_EVENT_PB_H_INCLUDED
#include "pb.h"

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _snova_AuthRequest {
  char user[256];
  uint64_t client_id;
  bool is_entry;
  bool is_exit;
  bool is_middle;
} snova_AuthRequest;

typedef struct _snova_AuthResponse {
  bool success;
  uint64_t iv;
} snova_AuthResponse;

typedef struct _snova_StreamOpenRequest {
  char remote_host[512];
  uint32_t remote_port;
  bool is_tcp;
  bool is_tls;
} snova_StreamOpenRequest;

#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define snova_AuthRequest_init_default \
  { "", 0, 0, 0, 0 }
#define snova_AuthResponse_init_default \
  { 0, 0 }
#define snova_StreamOpenRequest_init_default \
  { "", 0, 0, 0 }
#define snova_AuthRequest_init_zero \
  { "", 0, 0, 0, 0 }
#define snova_AuthResponse_init_zero \
  { 0, 0 }
#define snova_StreamOpenRequest_init_zero \
  { "", 0, 0, 0 }

/* Field tags (for use in manual encoding/decoding) */
#define snova_AuthRequest_user_tag 1
#define snova_AuthRequest_client_id_tag 2
#define snova_AuthRequest_is_entry_tag 3
#define snova_AuthRequest_is_exit_tag 4
#define snova_AuthRequest_is_middle_tag 5
#define snova_AuthResponse_success_tag 1
#define snova_AuthResponse_iv_tag 2
#define snova_StreamOpenRequest_remote_host_tag 1
#define snova_StreamOpenRequest_remote_port_tag 2
#define snova_StreamOpenRequest_is_tcp_tag 3
#define snova_StreamOpenRequest_is_tls_tag 4

/* Struct field encoding specification for nanopb */
#define snova_AuthRequest_FIELDLIST(X, a)      \
  X(a, STATIC, SINGULAR, STRING, user, 1)      \
  X(a, STATIC, SINGULAR, UINT64, client_id, 2) \
  X(a, STATIC, SINGULAR, BOOL, is_entry, 3)    \
  X(a, STATIC, SINGULAR, BOOL, is_exit, 4)     \
  X(a, STATIC, SINGULAR, BOOL, is_middle, 5)
#define snova_AuthRequest_CALLBACK NULL
#define snova_AuthRequest_DEFAULT NULL

#define snova_AuthResponse_FIELDLIST(X, a) \
  X(a, STATIC, SINGULAR, BOOL, success, 1) \
  X(a, STATIC, SINGULAR, UINT64, iv, 2)
#define snova_AuthResponse_CALLBACK NULL
#define snova_AuthResponse_DEFAULT NULL

#define snova_StreamOpenRequest_FIELDLIST(X, a)  \
  X(a, STATIC, SINGULAR, STRING, remote_host, 1) \
  X(a, STATIC, SINGULAR, UINT32, remote_port, 2) \
  X(a, STATIC, SINGULAR, BOOL, is_tcp, 3)        \
  X(a, STATIC, SINGULAR, BOOL, is_tls, 4)
#define snova_StreamOpenRequest_CALLBACK NULL
#define snova_StreamOpenRequest_DEFAULT NULL

extern const pb_msgdesc_t snova_AuthRequest_msg;
extern const pb_msgdesc_t snova_AuthResponse_msg;
extern const pb_msgdesc_t snova_StreamOpenRequest_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define snova_AuthRequest_fields &snova_AuthRequest_msg
#define snova_AuthResponse_fields &snova_AuthResponse_msg
#define snova_StreamOpenRequest_fields &snova_StreamOpenRequest_msg

/* Maximum encoded size of messages (where known) */
#define snova_AuthRequest_size 275
#define snova_AuthResponse_size 13
#define snova_StreamOpenRequest_size 524

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif