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
	class FrameworkManager;
	class IAlgorithm;
}

namespace PythonAPI
{

class DLLExport PythonInterface
{
private:
	Mantid::API::FrameworkManager* fwMgr;

public:
	PythonInterface();
	~PythonInterface();

	void InitialiseFrameworkManager();

	//Algorithms
	bool CreateAlgorithm(const std::string&);
	bool ExecuteAlgorithm(const std::string&, const std::string&);

	//Load Data
	bool LoadNexusFile(const std::string&, const std::string&);
	int LoadIsisRawFile(const std::string&, const std::string&);

	std::vector<double> GetXData(const std::string&, int const );
	std::vector<double> GetYData(const std::string&, int const );

	//Needed for QTIPLOT
	unsigned long GetAddressXData(const std::string&, int const );
	unsigned long GetAddressYData(const std::string&, int const );
	

};

}
}

#endif //MANTID_PYTHONAPI_PYTHONINTERFACE_H_

