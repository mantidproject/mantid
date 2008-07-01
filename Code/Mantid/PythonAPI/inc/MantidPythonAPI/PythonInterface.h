#ifndef MANTID_PYTHONAPI_PYTHONINTERFACE_H_
#define MANTID_PYTHONAPI_PYTHONINTERFACE_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/System.h"

namespace Mantid
{

namespace API
{
class IAlgorithm;
class Workspace;
}

namespace PythonAPI
{
	//Namespace functions
	DLLExport boost::shared_ptr<API::Workspace> LoadIsisRawFile(const std::string&, const std::string&);
	DLLExport std::vector<std::string> GetAlgorithmNames();
	DLLExport std::vector<std::string> GetWorkspaceNames();

}
}

#endif //MANTID_PYTHONAPI_PYTHONINTERFACE_H_
