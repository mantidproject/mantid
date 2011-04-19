/** 

    @file idc.cpp 
    
    IDC interface - minimal socket interface to the DAE
    
    @author Freddie Akeroyd, STFC ISIS Facility
    @date 31/07/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

    This file is part of ISIS Instrument control program.
    you can redistribute it and/or modify it under the terms of the 
    GNU General Public License
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "idc.h"
#include "isisds_command.h"

//suppress warnings that say that vsprintf has been deprecated
#define _CRT_SECURE_NO_WARNINGS

///@cond nodoc
/** used to keep status information about the DAE connection */
struct idc_info
{
	SOCKET s; ///< The socket
};

/** default function used by IDCreport() - changed by calling IDCsetreportfunc() */
static void default_status_reporter(int status, int code, const char* message)
{
	printf("IDC: %d %d %s\n", status, code, message);
}

/** pointer to current IDC error reporing function */
static idc_error_report_t status_reporter = default_status_reporter;

// diable warning about vsprintf and strncpy
#pragma warning(disable:4996)
/** report an error */
int IDCreport(int status, int code, const char* format, ... )
{
	va_list ap;
	char* message = (char*)malloc(1024);
	va_start(ap, format);
	vsprintf(message, format, ap);
	va_end(ap);
    (*status_reporter)(status, code, message);
	free(message);
	return 0;
}

/** change the error report function */
int IDCsetreportfunc(idc_error_report_t report_func)
{
	status_reporter = report_func;
	return 0;
}

/** returns 0 on success, -1 on failure */
int IDCopen(const char* host, int mode, int options, idc_handle_t* pfh)
{
  (void) mode; // Avoid compiler warning
  (void) options; // Avoid compiler warning

	SOCKET s;
	*pfh = NULL;
	s = isisds_send_open(host, ISISDSDAEAccess);
	if (s == INVALID_SOCKET)
	{
		IDCreport(0, 0, "Error accessing DAE");
		return -1;
	}
	(*pfh) = (struct idc_info*)malloc(sizeof(struct idc_info));
	(*pfh)->s = s;
	return 0;
}
 ///Close the IDC
int IDCclose(idc_handle_t* pfh)
{
	isisds_send_close((*pfh)->s);
	free((*pfh));
	*pfh = NULL;
	return 0;
}

/** main worker routine for all data reading. Returns 0 on success, -1 on error */
static int getdat(idc_handle_t fh, int ifsn, int nos, int** value, int dims_array[], int* ndims, int do_alloc)
{
        (void) isisds_type_size; // Avoid compiler warning
        (void) isisds_type_name; // Avoid compiler warning

	int stat, comm_buff_size;
	ISISDSDataType ret_type;
	int spec_nos[2] = { ifsn, nos };
	int spec_nos_dims[1] = { 2 };
	char* command = NULL;
	char comm_buffer[256];
	if (isisds_send_command(fh->s, "GETDAT", spec_nos, ISISDSInt32, spec_nos_dims, 1) <= 0)
	{
		IDCreport(0, 0, "error sending command (getdat)");
		return -1;
	};
        ret_type = ISISDSInt32;
	if (do_alloc)
	{
		stat = isisds_recv_command_alloc(fh->s, &command, (void**)value, &ret_type, dims_array, ndims);
	}
	else
	{
		comm_buff_size = sizeof(comm_buffer);
		stat = isisds_recv_command(fh->s, comm_buffer, &comm_buff_size, *value, &ret_type, dims_array, ndims);
	}
	if (stat <= 0)
	{
		IDCreport(0, 0, "error reading command (getdat)");
		return -1;
	}
	if (ret_type != ISISDSInt32)
	{
		IDCreport(0, 0, "invalid return type command (getdat)");
		return -1;
	}
/*	IDCreport(0, 0, "type %s\n", isisds_type_name[ret_type]); */
	return 0;
}

int IDCgetdat(idc_handle_t fh, int ifsn, int nos, int* value, int dims_array[], int* ndims)
{
	return getdat(fh, ifsn, nos, &value, dims_array, ndims, 0);
}

int IDCAgetdat(idc_handle_t fh, int ifsn, int nos, int** value, int dims_array[], int* ndims)
{
	return getdat(fh, ifsn, nos, value, dims_array, ndims, 1);
}


///Get a parameter
static int IDCgetpar(idc_handle_t fh, const char* name, void** value, ISISDSDataType type,
					 int dims_array[], int* ndims, int do_alloc)
{
	int n, stat, comm_buff_size;
	ISISDSDataType ret_type;
	char* command = NULL;
	char comm_buffer[256];
	sprintf(comm_buffer, "GETPAR%s", isisds_type_code[type]);
	n = static_cast<int>(strlen(name));
	if (isisds_send_command(fh->s, comm_buffer, name, ISISDSChar, &n, 1) <= 0)
	{
		IDCreport(0, 0, "error sending command %s (getpar)", name);
		return -1;
	};
	ret_type = type;
	if (do_alloc)
	{
		stat = isisds_recv_command_alloc(fh->s, &command, value, &ret_type, dims_array, ndims);
	}
	else
	{
		comm_buff_size = sizeof(comm_buffer);
		stat = isisds_recv_command(fh->s, comm_buffer, &comm_buff_size, *value, &ret_type, dims_array, ndims);
	}
	if (stat <= 0)
	{
		IDCreport(0, 0, "error receiving command %s (getpar)", name);
		return -1;
	}
/*	IDCreport(0, 0, "type %s\n", isisds_type_name[ret_type]); */
	if (ret_type != type)
	{
	    return -1;
	}
	return 0;
}

///Get a parameter
int IDCAgetpari(idc_handle_t fh, const char* name, int** value, int dims_array[], int* ndims)
{
	int stat;
	ISISDSDataType type = ISISDSInt32;
	stat = IDCgetpar(fh, name, (void**)value, type, dims_array, ndims, 1);
	return stat;
}

///Get a parameter
int IDCgetpari(idc_handle_t fh, const char* name, int* value, int dims_array[], int* ndims)
{
	int stat;
	ISISDSDataType type = ISISDSInt32;
	stat = IDCgetpar(fh, name, (void**)&value, type, dims_array, ndims, 0);
	return stat;
}

///Get a parameter
int IDCgetparr(idc_handle_t fh, const char* name, float* value, int dims_array[], int* ndims)
{
	int stat;
	ISISDSDataType type = ISISDSReal32;
	stat = IDCgetpar(fh, name, (void**)&value, type, dims_array, ndims, 0);
	return stat;
}

///Get a parameter
int IDCAgetparr(idc_handle_t fh, const char* name, float** value, int dims_array[], int* ndims)
{
	int stat;
	ISISDSDataType type = ISISDSReal32;
	stat = IDCgetpar(fh, name, (void**)value, type, dims_array, ndims, 1);
	return stat;
}

///Get a parameter
int IDCgetpard(idc_handle_t fh, const char* name, double* value, int dims_array[], int* ndims)
{
	int stat;
	ISISDSDataType type = ISISDSReal64;
	stat = IDCgetpar(fh, name, (void**)&value, type, dims_array, ndims, 0);
	return stat;
}

///Get a parameter
int IDCAgetpard(idc_handle_t fh, const char* name, double** value, int dims_array[], int* ndims)
{
	int stat;
	ISISDSDataType type = ISISDSReal64;
	stat = IDCgetpar(fh, name, (void**)value, type, dims_array, ndims, 1);
	return stat;
}

///Get a parameter
int IDCgetparc(idc_handle_t fh, const char* name, char* value, int dims_array[], int* ndims)
{
	int stat;
	ISISDSDataType type = ISISDSChar;
	stat = IDCgetpar(fh, name, (void**)&value, type, dims_array, ndims, 0);
	return stat;
}

///Get a parameter
int IDCAgetparc(idc_handle_t fh, const char* name, char** value, int dims_array[], int* ndims)
{
	int stat;
	ISISDSDataType type = ISISDSChar;
	stat = IDCgetpar(fh, name, (void**)value, type, dims_array, ndims, 1);
	return stat;
}


#ifdef _WIN32
void __stdcall IDCFOPEN(const char* host, unsigned len_host, int* mode, int* options, int fh[], int* errcode)
#else
void idcfopen_(const char* host, int* mode, int* options, int fh[], int* errcode, unsigned len_host)
#endif
{
	int stat;
	idc_handle_t tfh;
	char t_host[256];
	strncpy(t_host, host, len_host);
	t_host[len_host] = '\0';
	stat = IDCopen(t_host, *mode, *options, &tfh);
	memcpy(fh, &tfh, sizeof(idc_handle_t));
	*errcode = stat;
}

#ifdef _WIN32
void __stdcall IDCFCLOSE(int fh[], int* errcode)
#else
void idcfclose_(int fh[], int* errcode)
#endif
{
	int stat;
	idc_handle_t tfh;
	memcpy(&tfh, fh, sizeof(idc_handle_t));
	stat = IDCclose(&tfh);
	memcpy(fh, &tfh, sizeof(idc_handle_t));
	*errcode = stat;
}

#ifdef _WIN32
void __stdcall IDCFGETPARI(int fh[], const char* name, unsigned len_name, int value[], int dims_array[], int* ndims,  int* errcode)
#else
void idcfgetpari_(int fh[], const char* name, int value[], int dims_array[], int* ndims,  int* errcode, unsigned len_name)
#endif
{
	int stat;
	char t_name[256];
	idc_handle_t tfh;
	strncpy(t_name, name, len_name);
	t_name[len_name] = '\0';
	memcpy(&tfh, fh, sizeof(idc_handle_t));
	stat = IDCgetpari(tfh, t_name, value, dims_array, ndims);
	memcpy(fh, &tfh, sizeof(idc_handle_t));
	*errcode = stat;
}

#ifdef _WIN32
void __stdcall IDCFGETPARR(int fh[], const char* name, unsigned len_name, float value[], int dims_array[], int* ndims,  int* errcode)
#else
void idcfgetparr_(int fh[], const char* name, float value[], int dims_array[], int* ndims,  int* errcode, unsigned len_name)
#endif
{
	int stat;
	char t_name[256];
	idc_handle_t tfh;
	strncpy(t_name, name, len_name);
	t_name[len_name] = '\0';
	memcpy(&tfh, fh, sizeof(idc_handle_t));
	stat = IDCgetparr(tfh, t_name, value, dims_array, ndims);
	memcpy(fh, &tfh, sizeof(idc_handle_t));
	*errcode = stat;
}

#ifdef _WIN32
void __stdcall IDCFGETPARD(int fh[], const char* name, unsigned len_name, double value[], int dims_array[], int* ndims,  int* errcode)
#else
void idcfgetpard_(int fh[], const char* name, double value[], int dims_array[], int* ndims,  int* errcode, unsigned len_name)
#endif
{
	int stat;
	char t_name[256];
	idc_handle_t tfh;
	strncpy(t_name, name, len_name);
	t_name[len_name] = '\0';
	memcpy(&tfh, fh, sizeof(idc_handle_t));
	stat = IDCgetpard(tfh, t_name, value, dims_array, ndims);
	memcpy(fh, &tfh, sizeof(idc_handle_t));
	*errcode = stat;
}

#ifdef _WIN32
void __stdcall IDCFGETPARC(int fh[], const char* name, unsigned len_name, char value[], unsigned len_value, int dims_array[], int* ndims,  int* errcode)
#else
void idcfgetparc_(int fh[], const char* name, char* value, int dims_array[], int* ndims,  int* errcode, unsigned len_name, unsigned len_value)
#endif
{
  (void) len_value; // Avoid compiler warning  

	int stat;
	char t_name[256];
	idc_handle_t tfh;
        if (len_name > 255)
        {
	    len_name = 255;
        }
	strncpy(t_name, name, len_name);
	t_name[len_name] = '\0';
	memcpy(&tfh, fh, sizeof(idc_handle_t));
	stat = IDCgetparc(tfh, t_name, value, dims_array, ndims);
	memcpy(fh, &tfh, sizeof(idc_handle_t));
	*errcode = stat;
}

#ifdef _WIN32
void __stdcall IDCFGETDAT(int fh[], int* ifsn, int* nos, int value[], int dims_array[], int* ndims, int* errcode)
#else
void idcfgetdat_(int fh[], int* ifsn, int* nos, int value[], int dims_array[], int* ndims, int* errcode)
#endif
{
	int stat;
	idc_handle_t tfh;
	memcpy(&tfh, fh, sizeof(idc_handle_t));
	stat = IDCgetdat(tfh, *ifsn, *nos, value, dims_array, ndims);
	memcpy(fh, &tfh, sizeof(idc_handle_t));
	*errcode = stat;
}


///@endcond
