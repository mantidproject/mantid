/** 

    @file isisds_command.h 
    
    IDC interface - minimal socket interface to the DAE
    
    @author Freddie Akeroyd, STFC ISIS Facility
    @date 31/07/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

    This file is part of ISIS Instrument control program.
    you can redistribute it and/or modify it under the terms of the 
    GNU General Public License
*/


#ifndef ISISDS_COMMAND_H
#define ISISDS_COMMAND_H

typedef void (*isisds_error_report_t)(int status, int code, const char* messsage);

#define ISISDS_PORT 6789

#ifdef _WIN32
#include <winsock2.h>
#else
#define SOCKET	int
#define INVALID_SOCKET	-1
#endif /* _WIN32 */

typedef enum { ISISDSDAEAccess = 0, ISISDSCRPTAccess = 1} ISISDSAccessMode;

typedef enum { ISISDSUnknown = 0, ISISDSInt32 = 1, ISISDSReal32 = 2, ISISDSReal64 = 3, ISISDSChar = 4 } ISISDSDataType;
static int isisds_type_size[] = { 0, 4, 4, 8, 1 };
static const char* isisds_type_name[] = { "Unknown", "Int32", "Real32", "Real64", "Char" };
static const char* isisds_type_code[] = { "U00", "I32", "R32", "R64", "C08" }; /* 3 char in length */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

SOCKET isisds_send_open(const char* host, ISISDSAccessMode access_type);
int isisds_recv_open(SOCKET s, ISISDSAccessMode* access_type);
int isisds_send_command(SOCKET s, const char* command, const void* data, ISISDSDataType type, const int dims_array[], int ndims);
int isisds_recv_command_alloc(SOCKET s, char** command, void** data, ISISDSDataType* type, int dims_array[], int* ndims);
int isisds_recv_command(SOCKET s, char* command, int* len_command, void* data, ISISDSDataType* type, int dims_array[], int* ndims);
int isisds_send_close(SOCKET s);

int isisds_set_report_func(isisds_error_report_t report_func);
int isisds_report(int status, int code, const char* format, ... );

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* 
 * try to align to 64 bit (8 bytes) boundaries
 */
typedef struct
{
	int len;
	int ver_major;
	int ver_minor;
	int pid;
	int access_type; /* 0 =dae, 1 = crpt */
	int pad[1];
	char user[32];
	char host[64];
}  isisds_open_t;

/*
 * used for sends and replies once a connection open
 * try to align to 64 bits (8 bytes) boundaries 
 */
typedef struct 
{
	int len;  /* of this structure plus any additional data (in bytes) */
 	int type; /* ISISDSDataType */
	int ndims;
	int dims_array[11];
	char command[32];
	/* additional data (if any) will follow this */
} isisds_command_header_t;

#endif /* ISISDS_COMMAND_H */
