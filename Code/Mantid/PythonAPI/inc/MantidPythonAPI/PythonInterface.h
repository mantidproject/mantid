#ifndef MANTID_PYTHONAPI_PYTHONINTERFACE_H_
#define MANTID_PYTHONAPI_PYTHONINTERFACE_H_

//---------------------------------------
//Includes
//---------------------------------------
#include <string>
#include <vector>
#include "MantidKernel/System.h"

namespace Mantid
{
namespace PythonAPI
{
	//Namespace functions
	DLLExport std::vector<std::string> GetAlgorithmNames();
	DLLExport std::vector<std::string> GetWorkspaceNames();
  DLLExport void createPythonSimpleAPI();

}
}

#endif //MANTID_PYTHONAPI_PYTHONINTERFACE_H_
