#ifndef MANTID_DATAHANDLING_LOADSPICEXML2DDET_H_
#define MANTID_DATAHANDLING_LOADSPICEXML2DDET_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace DataHandling {

/** LoadSpiceXML2DDet : Load 2D detector data in XML format form SPICE

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadSpiceXML2DDet : public API::Algorithm {
public:
  LoadSpiceXML2DDet();
  virtual ~LoadSpiceXML2DDet();

  virtual const std::string name() const { return "LoadSpiceXML2DDet"; }
  virtual int version() const { return 1; }
  virtual const std::string category() const { return "DataHandling"; }
  virtual const std::string summary() const {
    return "Load 2-dimensional detector data file in XML format from SPICE. ";
  }

private:
  void init();
  void exec();

  ///
  void parseSpiceXML(const std::string &xmlfilename);

  ///
  API::MatrixWorkspace_sptr createMatrixWorkspace();
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADSPICEXML2DDET_H_ */
