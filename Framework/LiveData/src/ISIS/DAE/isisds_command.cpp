/**

    @file isisds_command.cpp

    IDC interface - minimal socket interface to the DAE

    @author Freddie Akeroyd, STFC ISIS Facility
    @date 31/07/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

    This file is part of ISIS Instrument control program.
    you can redistribute it and/or modify it under the terms of the
    GNU General Public License
*/

// We aren't going to EVER change this code - so let's ignore the warnings.
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wformat"
#elif defined(_WIN32)
#pragma warning(disable : 4100)
// disable warning about vsprintf and strncpy
#pragma warning(disable : 4996)
#endif

///@cond nodoc

#include "isisds_command.h"
#include "MantidKernel/ConfigService.h"

#include <cstdio>

/*
 * versions of these structures
 * increment major for incompatible changes
 * increment minor for backward compatible changes on server
 */
#define ISISDS_MAJOR_VER 1
#define ISISDS_MINOR_VER 1

#ifndef _WIN32
#define closesocket close
#include <arpa/inet.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

/** Try to align to 64 bit (8 bytes) boundaries
 */
using isisds_open_t = struct {
  int len;
  int ver_major;
  int ver_minor;
  int pid;
  int access_type; /* 0 =dae, 1 = crpt */
  int pad[1];
  char user[32];
  char host[64];
};

/** used for sends and replies once a connection open
 * try to align to 64 bits (8 bytes) boundaries
 */
using isisds_command_header_t = struct {
  int len;  /* of this structure plus any additional data (in bytes) */
  int type; /* ISISDSDataType */
  int ndims;
  int dims_array[11];
  char command[32];
  /* additional data (if any) will follow this */
};

/* wait until read len bytes, return <=0 on error */
static int recv_all(SOCKET s, void *buffer, int len, int flags) {
  auto *cbuffer = reinterpret_cast<char *>(buffer);
  int ntot;
  ntot = 0;
  while (len > 0) {
    int n = recv(s, cbuffer, len, flags);
    if (n <= 0) {
      return n; /* error */
    }
    len -= n;
    cbuffer += n;
    ntot += n;
  }
  return ntot;
}

/* clear out old data from a socket */
static void clear_replies(SOCKET s) {
  static char buffer[100000];
  struct timeval timeout = {0, 0};
  fd_set fds;
  int done = 0;
  while (!done) {
    FD_ZERO(&fds);
    FD_SET(s, &fds);
    if ((select(FD_SETSIZE, &fds, nullptr, nullptr, &timeout) > 0) && FD_ISSET(s, &fds)) {
      recv(s, buffer, sizeof(buffer), 0);
    } else {
      done = 1;
    }
  }
}

/*
 * client: open a socket and perform initial negotiation
 * return connected socket or INVALID_SOCKET on error
 */
SOCKET isisds_send_open(const char *host, ISISDSAccessMode access_type, uint16_t port) {
  SOCKET s;
  int setkeepalive = 1;
  struct hostent *hostp;
  struct sockaddr_in address;
  char *comm, *comm_data;
  /*	int len_comm_data; */
  isisds_open_t op;
  ISISDSDataType data_type;
  int dims_array[10], ndims;

  if ((hostp = gethostbyname(host)) == nullptr) {
    return INVALID_SOCKET;
  }
  memset(&address, 0, sizeof(address));
  memcpy(&(address.sin_addr.s_addr), hostp->h_addr_list[0], hostp->h_length);
  address.sin_family = AF_INET;

  address.sin_port = htons(port);
  s = socket(PF_INET, SOCK_STREAM, 0);
  if (s == INVALID_SOCKET) {
    return INVALID_SOCKET;
  }

  int zero = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char *>(&setkeepalive), sizeof(setkeepalive));
  if (0 != zero) {
    closesocket(s);
    return INVALID_SOCKET;
  }

  if (connect(s, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) == -1) {
    closesocket(s);
    return INVALID_SOCKET;
  }

  auto timeoutInSecConfigVal = Mantid::Kernel::ConfigService::Instance().getValue<int>("ISISDAE.Timeout");
  int timeoutinSec = timeoutInSecConfigVal.value_or(120); // Default to  120 seconds if not specified

#ifdef WIN32
  // WINDOWS
  DWORD timeout = timeoutinSec * 1000;
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#else
  // LINUX
  struct timeval tv;
  tv.tv_sec = timeoutinSec; /* 30 Secs Timeout */
  tv.tv_usec = 0;           // Not init'ing this can cause strange errors
  setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(struct timeval));
#endif // WIN

  /* socket connected */
  op.ver_major = ISISDS_MAJOR_VER;
  op.ver_minor = ISISDS_MINOR_VER;
  op.pid = 0;
  op.pad[0] = 0;
  op.access_type = access_type;
  strncpy(op.user, "faa", sizeof(op.user));
  strncpy(op.host, "localhost", sizeof(op.host));
  op.len = sizeof(op);
  if ((send(s, reinterpret_cast<char *>(&op), sizeof(op), 0)) != sizeof(op)) {
    closesocket(s);
    return INVALID_SOCKET;
  }
  comm = nullptr;
  if (isisds_recv_command_alloc(s, &comm, reinterpret_cast<void **>(&comm_data), &data_type, dims_array, &ndims) <= 0) {
    closesocket(s);
    free(comm);
    return INVALID_SOCKET;
  }
  if (comm_data != nullptr) {
    free(comm_data);
  }
  if (!strcmp(comm, "OK")) {
    free(comm);
    return s;
  } else {
    free(comm);
    closesocket(s);
    return INVALID_SOCKET;
  }
}

/*
 * server
 * client minor ver <= server minor ver
 */
int isisds_recv_open(SOCKET s, ISISDSAccessMode *access_type) {
  isisds_open_t op;
  if ((recv_all(s, reinterpret_cast<char *>(&op), sizeof(op), 0)) != sizeof(op)) {
    return -1;
  }
  if (op.len != sizeof(op)) {
    return -1;
  }
  if ((op.ver_major != ISISDS_MAJOR_VER) || (op.ver_minor > ISISDS_MINOR_VER)) {
    return -1;
  }
  *access_type = static_cast<ISISDSAccessMode>(op.access_type);
  return isisds_send_command(s, "OK", nullptr, ISISDSUnknown, nullptr, 0);
}
/*
 * return > 0 on success
 * if dims_array == NULL, ndims is length
 * on graceful termination should get an FD_CLOSE from client, should then send
 * any data
 * shutdown(s, SD_SEND) and then closesocket()
 */
int isisds_send_command(SOCKET s, const char *command, const void *data, ISISDSDataType type, const int dims_array[],
                        int ndims) {

  (void)isisds_type_code; // Avoid compiler warning
  (void)isisds_type_name; // Avoid compiler warning

  int n, len_data;
  isisds_command_header_t comm;
  memset(&comm, 0, sizeof(comm));
  if (dims_array == nullptr) {
    comm.ndims = 1;
    comm.dims_array[0] = ndims;
    len_data = ndims * isisds_type_size[type];
  } else {
    int i;
    len_data = 1;
    comm.ndims = ndims;
    for (i = 0; i < ndims; i++) {
      len_data *= dims_array[i];
      comm.dims_array[i] = dims_array[i];
    }
    len_data *= isisds_type_size[type];
  }
  comm.len = sizeof(comm) + len_data;
  comm.type = type;
  // fixing coverity warning: comm.command is filled with 0's by memset(&comm,
  // 0, sizeof(comm)); above
  // if strncpy reaches the limit the last character in comm.command is still
  // '\0'
  strncpy(comm.command, command, sizeof(comm.command) - 1);
  clear_replies(s);
  n = send(s, reinterpret_cast<char *>(&comm), sizeof(comm), 0);
  if ((n == sizeof(comm)) && (data != nullptr) && (len_data > 0)) {
    n = send(s, reinterpret_cast<const char *>(data), len_data, 0);
  }
  return n;
}

/* if not do_alloc, then type and dims_array are checked */
static int isisds_recv_command_helper(SOCKET s, char **command, void **data, ISISDSDataType *type, int dims_array[],
                                      int *ndims, int do_alloc) {
  int n, len_data, i;
  isisds_command_header_t comm;
  n = recv_all(s, reinterpret_cast<char *>(&comm), sizeof(comm), 0);
  if (n != sizeof(comm)) {
    return -1;
  }
  const auto commCommandSize = sizeof(comm.command);
  *command = reinterpret_cast<char *>(malloc(commCommandSize + 1));
  strncpy(*command, comm.command, commCommandSize);
  (*command)[commCommandSize] = '\0';
  len_data = comm.len - sizeof(comm); /* in bytes */
  if (len_data < 0) {
    return -1; /* error */
  } else if (len_data == 0) {
    dims_array[0] = 0;
    /*		*ndims = 0; */
    *type = ISISDSUnknown;
    return n; /* all ok, just no extra data */
  }
  /*	isisds_report(0, 0, "Received data Type = \"%s\", ndims = %d\n",
   * isisds_type_name[comm.type], comm.ndims); */
  if (do_alloc) {
    *data = malloc(len_data + 1);
    (reinterpret_cast<char *>(*data))[len_data] = '\0';
  } else {
    int size_in = 1;
    for (i = 0; i < *ndims; i++) {
      size_in *= dims_array[i];
    }
    size_in *= isisds_type_size[*type];
    if (size_in < len_data) {
      isisds_report(0, 0, "data array too small %d < %d\n", size_in, len_data);
      return -1;
    }
    if (size_in > len_data) /* only NULL terminate if space */
    {
      (reinterpret_cast<char *>(*data))[len_data] = '\0';
    }
  }
  n = recv_all(s, *data, len_data, 0);
  if (n != len_data) {
    if (do_alloc) {
      free(*data);
      *data = nullptr;
    }
    return -1;
  }
  /* only update values if changed ... allows Read only parameters to be passed
   */
  if (do_alloc || (*ndims != comm.ndims)) {
    *ndims = comm.ndims;
  }
  if (do_alloc || (*type != comm.type)) {
    *type = static_cast<ISISDSDataType>(comm.type);
  }
  for (i = 0; i < comm.ndims; i++) {
    dims_array[i] = comm.dims_array[i];
  }
  return n;
}

/* return > 0 on success */
int isisds_recv_command(SOCKET s, char *command, int *len_command, void *data, ISISDSDataType *type, int dims_array[],
                        int *ndims) {
  int t_ndims = 1;
  int istat;
  char *command_temp = nullptr;
  if (type == nullptr) {
    return -1;
  }
  if (dims_array == nullptr || ndims == nullptr || (*ndims <= 1 && dims_array[0] <= 1)) {
    int t_dims[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    /* assume single simple value */
    istat = isisds_recv_command_helper(s, &command_temp, &data, type, t_dims, &t_ndims, 0);
    if ((t_ndims != 1) || (t_dims[0] != 1)) {
      istat = -1;
    }
  } else {
    istat = isisds_recv_command_helper(s, &command_temp, &data, type, dims_array, ndims, 0);
  }
  if (command_temp) {
    strncpy(command, command_temp, *len_command);
    *len_command = static_cast<int>(strlen(command_temp));
    free(command_temp);
  }
  return istat;
}

/* return > 0 on success */
int isisds_recv_command_alloc(SOCKET s, char **command, void **data, ISISDSDataType *type, int dims_array[],
                              int *ndims) {
  if (ndims == nullptr || dims_array == nullptr || type == nullptr) {
    return -1;
  }
  if (data == nullptr || command == nullptr) {
    return -1;
  }
  *data = nullptr;
  *command = nullptr;
  /* *ndims = 0; */
  dims_array[0] = 0;
  *type = ISISDSUnknown;
  return isisds_recv_command_helper(s, command, data, type, dims_array, ndims, 1);
}

int isisds_send_close(SOCKET s) {
  /*	shutdown((*pfh)->s, SD_SEND);   indicate no more data to send SHUT_WR
   * check for FD_READ and recv any other stuff from server
   *check for FD_CLOSE and closesocket()
   */
  closesocket(s);
  return 0;
}

static void default_status_reporter(int status, int code, const char *message) {
  printf("ISISDS: %d %d %s\n", status, code, message);
}

static isisds_error_report_t status_reporter = default_status_reporter;

int isisds_report(int status, int code, const char *format, ...) {
  va_list ap;
  auto *message = reinterpret_cast<char *>(malloc(1024));
  va_start(ap, format);
  vsprintf(message, format, ap);
  va_end(ap);
  (*status_reporter)(status, code, message);
  free(message);
  return 0;
}

int isisds_set_report_func(isisds_error_report_t report_func) {
  status_reporter = report_func;
  return 0;
}

///@endcond
