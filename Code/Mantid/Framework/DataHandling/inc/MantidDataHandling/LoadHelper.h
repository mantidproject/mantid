#ifndef MANTID_DATAHANDLING_LoadHelper_H_
#define MANTID_DATAHANDLING_LoadHelper_H_

#include "MantidKernel/System.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadHelper : Auxiliary File for Loading Files

 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport LoadHelper {
public:
	LoadHelper();
	virtual ~LoadHelper();

	std::string findInstrumentNexusPath(const NeXus::NXEntry&);
	std::string getStringFromNexusPath(const NeXus::NXEntry&,
			const std::string&);
	double getDoubleFromNexusPath(const NeXus::NXEntry&, const std::string&);
	std::vector<double> getTimeBinningFromNexusPath(const NeXus::NXEntry &,
			const std::string &);
	static double calculateStandardError(double in) {
		return sqrt(in);
	}
	double calculateEnergy(double);
	double calculateTOF(double, double);
	double getL1(const API::MatrixWorkspace_sptr&);
	double getL2(const API::MatrixWorkspace_sptr&, int detId = 1);
	double getInstrumentProperty(const API::MatrixWorkspace_sptr&, std::string);
	std::string dateTimeInIsoFormat(std::string);
private:
	/// Reference to the logger class
	Kernel::Logger& g_log;
};

} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LoadHelper_H_ */
