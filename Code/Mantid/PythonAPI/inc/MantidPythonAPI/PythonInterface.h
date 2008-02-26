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
}

namespace PythonAPI
{

class DLLExport PythonInterface
{
public:
	PythonInterface();
	~PythonInterface();

	void InitialiseFrameworkManager();

	//Algorithms
	bool CreateAlgorithm(const std::string&);
	bool ExecuteAlgorithm(const std::string&, const std::string&);

	//Load Data
	int LoadIsisRawFile(const std::string&, const std::string&);

	//Access Data
	std::vector<double> GetXData(const std::string&, int const );
	std::vector<double> GetYData(const std::string&, int const );
	std::vector<double> GetEData(const std::string&, int const );
	std::vector<double> GetE2Data(const std::string&, int const );

	//Needed for QTIPLOT
	unsigned long GetAddressXData(const std::string&, int const );
	unsigned long GetAddressYData(const std::string&, int const );
	

};

}
}

#endif //MANTID_PYTHONAPI_PYTHONINTERFACE_H_

