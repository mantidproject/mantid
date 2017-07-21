#ifndef MANTID_DATAHANDLING_SAVESESANS_H_
#define MANTID_DATAHANDLING_SAVESESANS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

/** SaveSESANS : Save a workspace in the SESANS file format

  Require properties:
  <UL>
  <LI> InputWorkspace - The name of the workspace to save</LI>
  <LI> Filename - The path to save the file</LI>
  <LI> Theta_zmax - TODO : Find a good description for this</LI>
  <LI> Theta_zmaz_unit - Unit for theta_znmax</LI>
  <LI> Theta_ymax - TODO : Find a good description for this</LI>
  <LI> Theta_ymaz_unit - Unit for theta_ymax</LI>
  </UL>

  @author Joseph Ramsay, ISIS
  @date 19/07/2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class MANTID_DATAHANDLING_DLL SaveSESANS : public API::Algorithm {
public:
	const std::string name() const override;
	const std::string summary() const override;
	int version() const override;
	const std::string category() const override;

private:
	// Length of the longest attribute name in headers (+4 for readbility in the file)
	const int MAX_HDR_LENGTH = 23;
	const std::vector<std::string> fileExtensions{ ".ses" };

	void init() override;
	void exec() override;

	void writeHeaders(std::ofstream &outfile, API::MatrixWorkspace_const_sptr &ws);
	void writeHeader(std::ofstream &outfile, const std::string &name, const std::string &value);

};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVESESANS_H_ */