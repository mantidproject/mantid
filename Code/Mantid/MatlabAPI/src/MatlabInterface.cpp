#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "boost/algorithm/string.hpp"
#include "boost/pointer_cast.hpp"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"

#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataObjects/Workspace2D.h"

#include "mex.h"
/**     @file MatlabInterface.cpp
	MATLAB access to the mantid API

	@author Freddie Akeroyd, STFC Rutherford Appleton Laboratories
	@date 15/09/2008

	Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

	This file is part of Mantid.

	Mantid is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	Mantid is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
	Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/* 
 * The type  mexfunc_t  and the macro  declare_function()
 * must create the same function signature
 */

typedef int (*mexfunc_t)(int nlhs, mxArray *plhs[],
				int nrhs, const mxArray* prhs[]);

typedef struct
{
	const char* name;
	mexfunc_t func;
} mexfunc_s_t;

extern int CreateFrameworkManager(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
extern int CreateAlgorithm(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
extern int LoadRawRun(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
extern int WorkspaceGetField(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);

static mexfunc_s_t mex_functions[] = {
    { "FrameworkManager_Create", CreateFrameworkManager },
    { "Algorithm_Create", CreateAlgorithm },
    { "LoadRaw_Run", LoadRawRun },
    { "Workspace_GetField", WorkspaceGetField },
    { NULL, NULL }
};

/*
 * the mex function is called with the class name followed by the operation name
 * as the first two matlab arguments e.g. libisisexc("ixtestclass", "plus")
 * From this a FORTRAN function name is created (ixtestclass_plus) which is then called with 
 * the rest of the parameters
 */
#define BUFFER_LEN	64
#define MAX_ARGS	100

#ifdef _WIN32
#    define compare_nocase stricmp
#    define mwSize int
#    define uint64_t unsigned long
#else 
#    define compare_nocase strcasecmp
#endif

using namespace Mantid::API;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
	int i, j, n, nrhs_2, func_called = 0, errcode = 0;
	static int first_call = 0;
	static int call_depth = 0;
	char error_buffer[256];
	const mxArray* new_prhs[MAX_ARGS];
	char classname[BUFFER_LEN+1], classop[BUFFER_LEN+1], funcname[2*BUFFER_LEN+2];
	if (first_call == 0)
	{
		first_call = 1;
		mexLock();
	}
	if (nrhs < 2)
	{
		mexErrMsgTxt("LIBISISEXC: At least two arguments (\"class\", \"class operation\") are required");
	}
	if (nrhs >= MAX_ARGS)
	{
		mexErrMsgTxt("LIBISISEXC: too many varargin arguments");
	}
    if (mxGetString(prhs[0], classname, BUFFER_LEN) != 0)
	{
		mexErrMsgTxt("LIBISISEXC: cannot read argument 1 (class name)");
	}
    if (mxGetString(prhs[1], classop, BUFFER_LEN) != 0)
	{
		mexErrMsgTxt("LIBISISEXC: cannot read argument 2 (class operation name)");
	}
/*
 * NULLify out PLHS as we use this as a test to create them
 */
	for(i=0; i<nlhs; i++)
	{
		plhs[i] = NULL;
	}
    sprintf(funcname, "%s_%s", classname, classop);
	
/*
 * look for the special case of function name ending in _varargin
 * If we find this, flatten any cell arays we find in prhs (varargin arrays)
 * and then call the relevant function (i.e. the name without varargin)
 */
	i = strlen(funcname);
	if (!compare_nocase(funcname + (i - 9), "_varargin"))
	{
		funcname[i-9] = '\0';	/* remove the trailing "_varargin" from the name */
		n = 0;
		for(i=0; i<nrhs; i++)
		{
			if (mxIsCell(prhs[i]))
			{
				for(j=0; j < mxGetNumberOfElements(prhs[i]); j++)
				{
					new_prhs[n++] = mxGetCell(prhs[i], j);
					if (n >= MAX_ARGS)
					{
						mexErrMsgTxt("LIBISISEXC: too many varargin arguments");
					}
				}
			}
			else
			{
				new_prhs[n++] = prhs[i];
				if (n >= MAX_ARGS)
				{
					mexErrMsgTxt("LIBISISEXC: too many varargin arguments");
				}
			}
		}
		nrhs_2 = n - 2;
	}
	else
	{
		for(i=0; i<nrhs; i++)
		{
			new_prhs[i] = prhs[i];
		}
		nrhs_2 = nrhs - 2;
	}
	++call_depth;
	if (call_depth > 1)
	{
		call_depth = 0;	/* need to reset */
/*		mexErrMsgTxt("LIBISISEXC: NOT re-entrant"); */
		mexWarnMsgTxt("LIBISISEXC: Possible attempt to make re-entrant call");
		mexWarnMsgTxt("LIBISISEXC: This is often caused by a matlab class constructor not checking for nargin > 0");
	}
	for(i=0; i< (sizeof(mex_functions) / sizeof(mexfunc_s_t)) && !func_called; i++)
	{
		if ( (mex_functions[i].name != NULL) && !compare_nocase(funcname, mex_functions[i].name))
		{
			func_called = 1;
			errcode = (*mex_functions[i].func)(nlhs, plhs, nrhs_2, new_prhs+2);
/*			(*(mex_functions[i].func))(&nlhs, plhs, &nrhs_2, prhs+2); */
			if (errcode != 0)
			{
			    sprintf(error_buffer, "LIBISISEXC: error returned from function \"%s\"", funcname);
/*
				matlab will now stop after all errors coming from fortran DLL and not continue
				to reveres this change comments with the next line
*/				
				mexErrMsgTxt(error_buffer);
/*			    mexWarnMsgTxt(error_buffer);  */
			}
		}
	}
	--call_depth;
    if (!func_called)
	{
		sprintf(error_buffer, "LIBISISEXC: cannot find external function \"%s\"", funcname);
		mexErrMsgTxt(error_buffer);
	}
}


/* create object of given class */
mxArray* ixbcreateclassobject(const char* class_name)
{
    mxArray* plhs[1] = { NULL };
    if (mexCallMATLAB(1, plhs, 0, NULL, class_name) == 0)
    {
        return plhs[0];	/* SUCCESS */
    }
    else
    {
	return 0;
    }
}

/* create array of n objects of given class */

mxArray* ixbcreateclassarray(const char* class_name, int* n)
{
    mxArray* plhs[1] = { NULL };
    mxArray* prhs[2];
    prhs[0] = ixbcreateclassobject(class_name);
    prhs[1] = mxCreateDoubleScalar((double)(*n));
    if (mexCallMATLAB(1, plhs, 2, prhs, "extend") == 0)
    {
        return plhs[0];	/* SUCCESS */
    }
    else
    {
	return 0;
    }
}

int CreateFrameworkManager(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
    mwSize dims[2] = { 1, 1 };
    mexPrintf("Created FrameworkManager\n");
    FrameworkManager::Instance();
    plhs[0] = mxCreateNumericArray(2, dims, mxUINT64_CLASS, mxREAL);
    uint64_t* data = (uint64_t*)mxGetData(plhs[0]);
    data[0] = (uint64_t)5;
    return 0;
}

int CreateAlgorithm(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
	mwSize dims[2] = { 1, 1 };
        char algName[128];
	mxGetString(prhs[0], algName, sizeof(algName));
	IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm(algName);
	plhs[0] = mxCreateNumericArray(2, dims, mxUINT64_CLASS, mxREAL);
	uint64_t* data = (uint64_t*)mxGetData(plhs[0]);
	data[0] = (uint64_t)alg;
	return 0;
}

/**
 * Loads a standard ISIS raw file into Mantid, using the LoadRaw algorithm.
 **/
int LoadRawRun(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
        char fileName[128], work_name[128];
	mxGetString(prhs[0], fileName, sizeof(fileName));
	mxGetString(prhs[1], work_name, sizeof(work_name));
	//Check workspace does not exist
	if (!AnalysisDataService::Instance().doesExist(work_name))
	{
		IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("LoadRaw");
		alg->setPropertyValue("Filename", fileName);
		alg->setPropertyValue("OutputWorkspace", work_name);
		alg->execute();
	}
	else
	{
    		mexPrintf("Workspace already exists\n");
	}
	return 0;
}

int WorkspaceGetField(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
	std::vector<double>* data = NULL;
        char field_name[128], work_name[128];
	mxGetString(prhs[0], work_name, sizeof(work_name));
	mxGetString(prhs[1], field_name, sizeof(field_name));

	Workspace_sptr wksptr = AnalysisDataService::Instance().retrieve(work_name);
	switch(field_name[0])
	{
	    case 'x':
		data = &(wksptr->dataX(0));
		break;

	    case 'y':
		data = &(wksptr->dataY(0));
		break;

	    case 'e':
		data = &(wksptr->dataE(0));
		break;

	    default:
		break;

	}
	mwSize dims[2] = { 1, data->size() };
	plhs[0] = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
	memcpy(mxGetPr(plhs[0]), &(data->front()), data->size() * sizeof(double));
	return 0;
}

