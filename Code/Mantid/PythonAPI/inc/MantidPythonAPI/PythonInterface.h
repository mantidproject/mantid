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

/** @class PythonInterface PythonInterface.h PythonAPI/PythonInterface.h

    PythonInterface is the class through which it is possible to use Mantid from Python.

    @author ISIS, STFC
    @date 28/02/2008

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/
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
	int GetHistogramNumber(const std::string&);
	int GetBinNumber(const std::string&);

	//Access Data
	std::vector<double>* GetXData(const std::string&, int const );
	std::vector<double>* GetYData(const std::string&, int const );
	std::vector<double>* GetEData(const std::string&, int const );
	std::vector<double>* GetE2Data(const std::string&, int const );

	//Needed for QTIPLOT
	unsigned long GetAddressXData(const std::string&, int const );
	unsigned long GetAddressYData(const std::string&, int const );

};

}
}

#endif //MANTID_PYTHONAPI_PYTHONINTERFACE_H_
