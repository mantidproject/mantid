#ifndef IDC_H
#define IDC_H
/** 

    @file idc.h 
    
    IDC interface - minimal socket interface to the DAE
    
    @author Freddie Akeroyd, STFC ISIS Facility
    @date 31/07/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of ISIS Instrument control program.
    you can redistribute it and/or modify it under the terms of the 
    GNU General Public License
*/

///@cond nodoc


#include "isisds_command.h"

/** 
* holds information about the DAE connection - defined fully in idc.c
*/
struct idc_info;
typedef struct idc_info* idc_handle_t;

/**
 * prototype for error reporting function passed to IDCsetreportfunc() 
 */
typedef void (*idc_error_report_t)(int status, int code, const char* messsage);

#ifdef __cplusplus
extern "C" {
#endif

/** Open a DAE connection on host*/
int IDCopen(const char* host, int mode, int options, idc_handle_t* fh, uint16_t port = ISISDS_PORT);

/** Close a DAE connection */
int IDCclose(idc_handle_t* fh);

/* The A versions of the functions allocate memory, the other need to be passed a pre-allocated array */

/** Read an integer parameter \a name from the DAE and copy data into a pre-allocated array \a value.
    The size of value is held in the parameters dims_array and ndims  */
int IDCgetpari(idc_handle_t fh, const char* name, int* value, int dims_array[], int* ndims);

/** Read an integer parameter \a name from the DAE and allocate space of it in array \a value.
    The size of the returned array is written to the parameters dims_array and ndims  */
int IDCAgetpari(idc_handle_t fh, const char* name, int** value, int dims_array[], int* ndims);

/** Read a real (float) parameter \a name from the DAE and copy data into a pre-allocated array \a value.
    The size of value is held in the parameters dims_array and ndims  */
int IDCgetparr(idc_handle_t fh, const char* name, float* value, int dims_array[], int* ndims);

/** Read a real (float) parameter \a name from the DAE and allocate space of it in array \a value.
    The size of the returned array is written to the parameters dims_array and ndims  */
int IDCAgetparr(idc_handle_t fh, const char* name, float** value, int dims_array[], int* ndims);

/** Read a character parameter \a name from the DAE and copy data into a pre-allocated array \a value.
    The size of value is held in the parameters dims_array and ndims  */
int IDCgetpard(idc_handle_t fh, const char* name, double* value, int dims_array[], int* ndims);

/** Read a character parameter \a name from the DAE and allocate space of it in array \a value.
    The size of the returned array is written to the parameters dims_array and ndims  */
int IDCAgetparc(idc_handle_t fh, const char* name, char** value, int dims_array[], int* ndims);

/** Read a character parameter \a name from the DAE into pre-allocated array \a value.
    The size of the returned array is written to the parameters dims_array and ndims  */
int IDCgetparc(idc_handle_t fh, const char* name, char* value, int dims_array[], int* ndims);

/** Read \a nos spectra from the DAE starting at \a ifsn into pre-allocated array \a value.
    The size of value is held in the parameters \a dims_array and \a ndims  */
int IDCgetdat(idc_handle_t fh, int ifsn, int nos, int* value, int dims_array[], int* ndims);

/** Read \a nos spectra from the DAE starting at \a ifsn and allocate array \a value to return the results.
    The size of the returned array is written to the parameters \a dims_array and \a ndims  */
int IDCAgetdat(idc_handle_t fh, int ifsn, int nos, int** value, int dims_array[], int* ndims);

/** specify an error report function to be used by IDCreport() */ 
int IDCsetreportfunc(idc_error_report_t report_func);

/** Used to report an error by the IDC routines - it calls the function specified by IDCsetreportfunc()
  * to do the actual reporting */
int IDCreport(int status, int code, const char* format, ... );

///@endcond
#ifdef __cplusplus
}
#endif

#endif /* IDC_H */
