#include <fstream>
#include <Poco/File.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"

// Need this define to avoid a conflict in Visual Studio 2010
#if (_MSC_VER==1600) // This is MSVS2010
  #define CHAR16_T _CHAR16T
#endif
#include "engine.h"
/// A debugging define
#define ARGCHECK   // Also need mwdebug.c for this

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

/**
  * The type  mexfunc_t  and the macro  declare_function()
 *  must create the same function signature
 */
typedef int (*mexfunc_t)(int, mxArray**, int nrhs, const mxArray**);

/// A struct holding the name of a mex function and the function pointer to call
typedef struct
{
  ///The name of the function
	const char* name;
  /// The function pointer
	mexfunc_t func;
} mexfunc_s_t;

/// Create a FrameworkManager object
extern int CreateFrameworkManager(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// Get a workspace
extern int GetWorkspace(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// Create an algorithm
extern int CreateAlgorithm(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// Execute an algorithm
extern int RunAlgorithm(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// Execute an algorithm by giving a list of properties in a semi-colon separated list
extern int RunAlgorithmPV(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// Get a workspace field
extern int WorkspaceGetField(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// Get all fields in a worksace
extern int WorkspaceGetAllFields(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// Set a workspace field
extern int WorkspaceSetField(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// Create a simple API 
extern int CreateSimpleAPI(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// List the available workspaces
extern int ListWorkspaces(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);
/// Delete a workspace
extern int DeleteWorkspace(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[]);

/// An array mapping a string function name in Matlab, to one of the above functions
static mexfunc_s_t mex_functions[] = {
    { "FrameworkManager_Create", CreateFrameworkManager },
    { "FrameworkManager_GetWorkspace", GetWorkspace },
    { "FrameworkManager_DeleteWorkspace", DeleteWorkspace },
    { "Algorithm_Create", CreateAlgorithm },
    { "Algorithm_Run", RunAlgorithm },
    { "Algorithm_RunPV", RunAlgorithmPV },
    { "Workspace_GetField", WorkspaceGetField },
    { "Workspace_GetAllFields", WorkspaceGetAllFields },
    { "Workspace_SetField", WorkspaceSetField },
	  { "SimpleAPI_Create", CreateSimpleAPI },
    { "AnalysisDataService_ListWorkspaces", ListWorkspaces },
    { NULL, NULL }
};

/*
 *The mex function is called with the class name followed by the operation name
 * as the first two matlab arguments e.g. MantidMatlabAPI("ixtestclass", "plus")
 * From this a FORTRAN function name is created (ixtestclass_plus) which is then called with
 * the rest of the parameters
 */
/// Maximum bueffer length
#define BUFFER_LEN	64
/// Maximum number of arguments
#define MAX_ARGS	100

#ifdef _WIN32
  /// The function to use to compare case
  #define compare_nocase stricmp
  //starting from MATLAB R2006b (version 7.3.0)
  #if MX_API_VER <= 0x07020000 /* Version 7.2.0 */
  //mwSize and mwIndex are defined and must be used. However,
  /// these typedefs are not defined in previous MATLAB versions and must be defined 
  typedef int mwSize;
  typedef int mwIndex;
  #endif
  /// A 64-bit integer
  #define uint64_t UINT64
#else
  /// The function to use to compare case
  #define compare_nocase strcasecmp
#endif

using namespace Mantid::API;
using namespace Mantid::Kernel;

/**
  * Unroll a  cell from an array
  * @param prhs :: The right-hand side of function call in Matlab
  * @param new_prhs :: The new right-hand side after the function is complete
  * @param new_nrhs :: The number of parameters on the right-hand side afterwrads
  */
static void unrollCell(const mxArray *prhs, const mxArray* new_prhs[], int& new_nrhs)
{
	size_t j;
	if (mxIsCell(prhs))
	{
		for(j=0; j < mxGetNumberOfElements(prhs); j++)
		{
			unrollCell(mxGetCell(prhs, static_cast<int>(j)), new_prhs, new_nrhs);
		}
	}
	else
	{
		new_prhs[new_nrhs++] = prhs;
	}
}

/**
  * The main entry point that is called by Matlab when an external module's function is called
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters on the right-hand side of the equals 
  * @param prhs :: The data on the right-hand side of the equals
  */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
	int i, n, nrhs_2, func_called = 0, errcode = 0;
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
		mexErrMsgTxt("MANTIDEXC: At least two arguments (\"class\", \"class operation\") are required");
	}
	if (nrhs >= MAX_ARGS)
	{
		mexErrMsgTxt("MANTIDEXC: too many varargin arguments");
	}
    if (mxGetString(prhs[0], classname, BUFFER_LEN) != 0)
	{
		mexErrMsgTxt("MANTIDEXC: cannot read argument 1 (class name)");
	}
    if (mxGetString(prhs[1], classop, BUFFER_LEN) != 0)
	{
		mexErrMsgTxt("MANTIDEXC: cannot read argument 2 (class operation name)");
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
	i = static_cast<int>(strlen(funcname));
	if (!compare_nocase(funcname + (i - 9), "_varargin"))
	{
		funcname[i-9] = '\0';	/* remove the trailing "_varargin" from the name */
		n = 0;
		for(i=0; i<nrhs; i++)
		{
			unrollCell(prhs[i], new_prhs, n);
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
/*		mexErrMsgTxt("MANTIDEXC: NOT re-entrant"); */
		mexWarnMsgTxt("MANTIDEXC: Possible attempt to make re-entrant call");
		mexWarnMsgTxt("MANTIDEXC: This is often caused by a matlab class constructor not checking for nargin > 0");
	}
	for(i=0; i< static_cast<int>(sizeof(mex_functions) / sizeof(mexfunc_s_t)) && !func_called; i++)
	{
		if ( (mex_functions[i].name != NULL) && !compare_nocase(funcname, mex_functions[i].name))
		{
			func_called = 1;
			errcode = (*mex_functions[i].func)(nlhs, plhs, nrhs_2, new_prhs+2);
/*			(*(mex_functions[i].func))(&nlhs, plhs, &nrhs_2, prhs+2); */
			if (errcode != 0)
			{
			    sprintf(error_buffer, "MANTIDEXC: error returned from function \"%s\"", funcname);
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
		sprintf(error_buffer, "MANTIDEXC: cannot find external function \"%s\"", funcname);
		mexErrMsgTxt(error_buffer);
	}
}


/**
  * Create object of given class 
  * @param class_name :: The name of the class
  * @returns An array containing the created object
  */
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

/**
 * Create array of n objects of given class 
 * @param class_name :: The name of the class
 * @param n :: The number of objects to create
 * @returns An array of the created objects
 */
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

/**
  * Create a Framework manager object
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
  * @returns An integer indicating success/failure
  */
int CreateFrameworkManager(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
  mwSize dims[2] = {1, 1};
	try
	{
        FrameworkManagerImpl& fmgr = FrameworkManager::Instance();
        plhs[0] = mxCreateNumericArray(2, dims, mxUINT64_CLASS, mxREAL);
        uint64_t* data = (uint64_t*)mxGetData(plhs[0]);
        data[0] = (uint64_t)(&fmgr);   // dummy pointer to instance
        return 0;
	}
	catch(std::exception& e)
	{
    mexErrMsgTxt(e.what());
		return 1;
	}
}

/**
  * Get a workspace
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
   * @returns An integer indicating success/failure
   */
int GetWorkspace(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
  try
  {
    char buffer[256];
    mxGetString(prhs[0], buffer, sizeof(buffer));
    std::string wsName(buffer);
    Workspace* wksptr = FrameworkManager::Instance().getWorkspace(wsName);
    mwSize ndims[2] = {1, 1};
    plhs[0]=mxCreateNumericArray(2, ndims, mxUINT64_CLASS, mxREAL);
    uint64_t* data = (uint64_t*)mxGetData(plhs[0]);
    data[0] = (uint64_t)wksptr;
    return 0;
  }
  catch(std::exception& e)
  {
    mexErrMsgTxt(e.what());
		return 1;
  }
}

/**
  * Delete a workspace
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
  * @returns An integer indicating success/failure
  */
int DeleteWorkspace(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
  std::string wsName("");
  try
  {
    char buffer[256];
    mxGetString(prhs[0], buffer, sizeof(buffer));
    wsName = buffer;
    FrameworkManager::Instance().deleteWorkspace(wsName);
    return 0;
  }
  catch (Exception::NotFoundError&)
  {
    mexPrintf("A workspace with the name %s could not be found.\n", wsName.c_str());
		return 1;
  }

}

/**
  * Create an algorithm
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
   * @returns An integer indicating success/failure
   */
int CreateAlgorithm(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
	mwSize dims[2] = { 1, 1 };
    char algName[128];
	try
	{
	    mxGetString(prhs[0], algName, sizeof(algName));
	    IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm(algName);
	    plhs[0] = mxCreateNumericArray(2, dims, mxUINT64_CLASS, mxREAL);
	    uint64_t* data = (uint64_t*)mxGetData(plhs[0]);
	    data[0] = (uint64_t)alg;
		return 0;
	}
	catch(std::exception& e)
	{
    mexErrMsgTxt(e.what());
		return 1;
	}
}

/**
  * Execute an algorithm
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
 * @returns An integer indicating success/failure
   */
int RunAlgorithm(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
	char buffer[256];
	try
	{
	    uint64_t* data = (uint64_t*)mxGetData(prhs[0]);
	    IAlgorithm* alg = (IAlgorithm*)data[0];
      mxGetString(prhs[1], buffer, sizeof(buffer));
      alg->setProperties(buffer);
	    alg->execute();
	    plhs[0] = mxCreateString("");
	    return 0;
	}
	catch(std::exception& e)
	{
    mexErrMsgTxt(e.what());
		return 1;
	}

}

/**
  * Execute an algorithm with a property list
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
  * @returns An integer indicating success/failure
  */
int RunAlgorithmPV(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
	char buffer[256];
	mxArray* marray;
	std::string property_name;
	try
	{
	    uint64_t* data = (uint64_t*)mxGetData(prhs[0]);
	    IAlgorithm* alg = (IAlgorithm*)data[0];
	    int i=1;
	    while(i<nrhs)
	    {
		    if (mxGetClassID(prhs[i]) != mxCHAR_CLASS)
		    {
			    mexErrMsgTxt("Algorithm property name must be a string");
		    }
		    mxGetString(prhs[i], buffer, sizeof(buffer));
		    property_name = buffer;
		    i++;
			strcpy(buffer, mxGetClassName(prhs[i]));
			if (!strcmp(buffer, "char"))
			{
				mxGetString(prhs[i], buffer, sizeof(buffer));
				alg->setPropertyValue(property_name, buffer);
			}
			else if (!strcmp(buffer, "MantidWorkspace"))
			{
				marray = mxGetField(prhs[i],0,"name");
				mxGetString(marray, buffer, sizeof(buffer));
				alg->setPropertyValue(property_name, buffer);
			}
			else
			{
				mexErrMsgTxt("Algorithm property value must be a string");
			}
		    i++;
	    }
	    alg->execute();
	    plhs[0] = mxCreateString("");
	    return 0;
	}
	catch(std::exception& e)
	{
    mexErrMsgTxt(e.what());
		return 1;
	}
}

/**
  * Set a workspace field
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
  * @returns An integer indicating success/failure
  */
int WorkspaceSetField(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
	return 0;
}

/**
  *  A helper function to retrieve a values from a workspace
  * @param wksptr :: A pointer to a Mantid MatrixWorkspace
  * @param field :: A character indicating the field to return, i.e. x, y or e
  * @param ispec :: The spectrum number of the field to return
 * @returns  An array containing the data
  */
static mxArray* WorkspaceGetFieldHelper(MatrixWorkspace_sptr wksptr, char field, int ispec)
{
	const size_t nHist = wksptr->getNumberHistograms();
  mxArray* mptr = NULL;

	switch(field)
	{
    //remove iSpec
	  case 'x':
    {
      mwSize dims[2] = { static_cast<mwSize>(wksptr->dataX(0).size()),
        static_cast<mwSize>(nHist) };
      mptr = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
      char *start_of_pr = (char *)mxGetData(mptr);
      for ( int i = 0; i < wksptr->getNumberHistograms(); i++ )
      {
        size_t byteSize = wksptr->dataX(i).size()*sizeof(double);
        memcpy( start_of_pr, &(wksptr->dataX(i).front()), byteSize );
        start_of_pr += byteSize;
      }
    }
    break;

	  case 'y':
    {
      mwSize dims[2] = { static_cast<mwSize>(wksptr->dataY(0).size()),
        static_cast<mwSize>(nHist) };
      mptr = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
      char *start_of_pr = (char *)mxGetData(mptr);
      for ( int i = 0; i < wksptr->getNumberHistograms(); i++ )
      {
        size_t byteSize = wksptr->dataY(i).size()*sizeof(double);
        memcpy( start_of_pr, &(wksptr->dataY(i).front()), byteSize );
        start_of_pr += byteSize;
      }
    }
		break;

	  case 'e':
    {
      mwSize dims[2] = { static_cast<mwSize>(wksptr->dataE(0).size()),
        static_cast<mwSize>(nHist) };
      mptr = mxCreateNumericArray(2, dims, mxDOUBLE_CLASS, mxREAL);
      char *start_of_pr = (char *)mxGetData(mptr);
      for ( int i = 0; i < wksptr->getNumberHistograms(); i++ )
      {
        size_t byteSize = wksptr->dataE(i).size()*sizeof(double);
        memcpy( start_of_pr, &(wksptr->dataE(i).front()), byteSize );
        start_of_pr += byteSize;
      }
    }
		break;

	  default:
		return NULL;
	}
	return mptr;
}

/**
  * Get all  workspace fields
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
  * @returns An integer indicating success/failure
  */
int WorkspaceGetAllFields(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
  // char work_name[128];
	// mxGetString(prhs[0], work_name, sizeof(work_name));
	// mwSize dims_array[2] = { 1, 1 };
  // int nfields = 3;
	// char fieldnames[3] = { 'x', 'y', 'e' };
	// plhs[0] = mxCreateStructArray(2, dims_array, nfields, fieldnames);
	// Workspace_sptr wksptr = AnalysisDataService::Instance().retrieve(work_name);
	// mxArray* mptr;
	// mptr = WorkspaceGetFieldHelper(wksptr, 'x', 0);
	// mxSetField(plhs[0], 0, "x", mptr);
	// mptr = WorkspaceGetFieldHelper(wksptr, 'y', 0);
	// mxSetField(plhs[0], 0, "y", mptr);
	// mptr = WorkspaceGetFieldHelper(wksptr, 'e', 0);
	// mxSetField(plhs[0], 0, "e", mptr);
  mexErrMsgTxt("Error: This function has not been implemented yet");
	return 1;
}

/**
  * Get a workspace field
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
  * @returns An integer indicating success/failure
  */
int WorkspaceGetField(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
  char buffer[256];
  mxGetString(prhs[0], buffer, sizeof(buffer));
  std::string wsName(buffer);
	mxGetString(prhs[1], buffer, sizeof(buffer));
  std::string field(buffer);
  if( field.size() != 1 || (field[0] != 'x' && field[0] != 'y' && field[0] != 'e' ) )
  {
    mexErrMsgTxt("Error with field argument, must be either x, y or e");
  }
  double ispec(0);
  if( nrhs == 3 ) ispec = mxGetScalar(prhs[2]);
  MatrixWorkspace_sptr wksptr;
  try {
      wksptr = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName));
  }
  catch (Exception::NotFoundError&)
  {
    mexErrMsgTxt("The named workspace could not be found.");
	}

  mexPrintf("WorkspaceGetField %s %c %f \n", wsName.c_str(), field[0], ispec);

  plhs[0] = WorkspaceGetFieldHelper(wksptr, field[0], (int)ispec);
	return 0;
}

namespace
{
  /// Order the properties for the simple API
  struct PropertyOrdering
  {
    bool operator()(const Mantid::Kernel::Property * p1, const Mantid::Kernel::Property * p2) const
    {
  		//this is false, unless p1 is not valid and p2 is valid
	  	return ( p1->isValid() != "" ) && ( p2->isValid() == "" );
    }
  };
}

  /**
     * Take a property value as a string and if only special characters are present, i.e.
     * EOL characters then replace them with their string represenations
     * @param value :: The property value
     * @returns A string containing the sanitized property value
     */
  std::string santizePropertyValue(const std::string & value)
  {
    if( value == "\n\r" )
      return std::string("\\") + std::string("n") + std::string("\\") + std::string("r");
    if( value == "\n" )
      return std::string("\\") + std::string("n");
    return value;
  }

/**
  * A helper function to create the simple API
  * @param algName :: A string giving the name of the algorithm
  * @param path :: The path to the .m file that we should create
  */
void CreateSimpleAPIHelper(const std::string& algName, const std::string& path)
{
  IAlgorithm* alg;
  try {
    alg = FrameworkManager::Instance().createAlgorithm(algName);
  }
  catch(std::exception&)
  {
    std::string err = "An error occurred while writing the ";
    err += algName + " function definition.\n";
    mexErrMsgTxt(err.c_str());
    return;
  }
  std::string fullpath(path + algName + ".m");
  std::ofstream mfile(fullpath.c_str());

  typedef std::vector<Mantid::Kernel::Property*> PropertyVector;
  //parameter list
  mfile << "function res = " << algName << "(varargin)\n";
  //help string
  PropertyVector orderedProperties(alg->getProperties());
  std::sort(orderedProperties.begin(), orderedProperties.end(), PropertyOrdering());
  PropertyVector::const_iterator pIter = orderedProperties.begin();
  PropertyVector::const_iterator pEnd = orderedProperties.end();
  mfile << "%\t" << algName << "(";
  for( ; pIter != pEnd ; )
  {
    mfile << (*pIter)->name();
    if( ++pIter != pEnd ) mfile << ", ";
  }
  mfile << ")\n";
  mfile << "%\t\tArgument description:\n";
  pIter = orderedProperties.begin();
  unsigned int iOpt(0);
  for( ; pIter != pEnd ; ++pIter )
  {
    Mantid::Kernel::Property* prop = *pIter;
    mfile << "%\t\tName: " << prop->name() << ", Optional: ";
    if( prop->isValid() == "")
    {
      ++iOpt;
      mfile << "Yes, Default value: " << santizePropertyValue(prop->value());
    }
    else mfile << "No";
    mfile << ", Direction: " << Mantid::Kernel::Direction::asText(prop->direction());// << ", ";
    std::set<std::string> allowed = prop->allowedValues();
    if( !allowed.empty() )
    {
      mfile << ", Allowed values: ";
      std::set<std::string>::const_iterator sIter = allowed.begin();
      std::set<std::string>::const_iterator sEnd = allowed.end();
      for( ; sIter != sEnd ; )
      {
        mfile << (*sIter);
        if( ++sIter != sEnd ) mfile << ", ";
      }
    }
    mfile << "\n";
  }
  mfile << "%\n%\tNote: All string arguments must be wrapped in single quotes ''.\n";

  //The function definition
  mfile << "if nargin < " << (orderedProperties.size() - iOpt) << "\n"
        << "\tfprintf('All mandatory arguments have not been supplied, type \"help " << algName << "\" for more information\\n');\n"
        << "\treturn\n"
        << "end\n";

  mfile << "alg = MantidAlgorithm('" << algName << "');\n"
        << "argstring = '';\n";
  //Build arguments list
  mfile << "for i = 1:nargin\n"
        << "\targstring = strcat(argstring,varargin{i});\n"
        << "\tif i < nargin\n"
        << "\t\targstring = strcat(argstring, ';');\n"
        << "\tend\n"
        << "end\n";
  //Run the algorithm
  mfile << "res = run(alg, argstring);\n";
  mfile.close();
}

/**
  * Create the simple API
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
  * @returns An integer indicating success/failure
  */
int CreateSimpleAPI(int, mxArray **, int nrhs, const mxArray* prhs[])
{

  //Ensure all libraries are loaded
	FrameworkManager::Instance();

	//Create directory to store mfiles
	std::string mpath("");
  if( nrhs == 0 )
  {
    mpath = "MantidSimpleAPI";
  }
  else if( nrhs == 1 )
  {
    char buffer[256];
    mxGetString(prhs[0], buffer, sizeof(buffer));
    mpath = std::string(buffer) + "/MantidSimpleAPI";
  }
  else
  {
    mexErrMsgTxt("SimpleAPI_Create takes either 0 or 1 arguments.");
  }

  Poco::File simpleAPI( mpath );
  if(  simpleAPI.exists() )
  {
    simpleAPI.remove(true);
  }
  try
  {
    simpleAPI.createDirectory();
  }
  catch( std::exception& )
  {
    mexErrMsgTxt("An error occurred while creating the directory for the simple API.");
  }

  std::vector<std::string> algKeys = AlgorithmFactory::Instance().getKeys();
	std::vector<std::string>::const_iterator sIter = algKeys.begin();
	typedef std::map<std::string, unsigned int> VersionMap;
	VersionMap vMap;
	for( ; sIter != algKeys.end(); ++sIter )
	{
	  std::string key = (*sIter);
	  std::string name = key.substr(0, key.find("|"));
	  VersionMap::iterator vIter = vMap.find(name);
    if( vIter == vMap.end() ) vMap.insert(make_pair(name,1));
    else ++(vIter->second);
	}

  std::string contents_path = simpleAPI.path() + "/Contents.m";
  std::ofstream contents(contents_path.c_str());
  contents << "%A simpler API for Mantid\n%\n%The algorithms available are:\n";
  VersionMap::const_iterator vIter = vMap.begin();
  for( ; vIter != vMap.end(); ++vIter )
  {
    contents << "% " << vIter->first << "\n";
    CreateSimpleAPIHelper(vIter->first, mpath + std::string("/"));
  }
  contents << "% For help with an individual command type \"help algorithm_name\"\n";
  contents.close();
  return 0;
}

/**
  * List the available workspaces
  * @param nlhs :: The number of parameters on the left-hand side of the equals 
  * @param plhs :: The data on the left-hand side of the equals
  * @param nrhs :: The number of parameters in the Matlab function call
  * @param prhs :: The data from the Matlab function call
  * @returns An integer indicating success/failure
  */
int ListWorkspaces(int nlhs, mxArray *plhs[], int nrhs, const mxArray* prhs[])
{
  std::set<std::string> wkspNames = AnalysisDataService::Instance().getObjectNames();
  std::set<std::string>::const_iterator sEnd = wkspNames.end();
  //print the list of names using mexPrintf
  for( std::set<std::string>::const_iterator sIter = wkspNames.begin(); sIter != sEnd;
      ++sIter )
  {
    mexPrintf((*sIter).c_str());
    mexPrintf("\n");
  }
  return 0;
}
