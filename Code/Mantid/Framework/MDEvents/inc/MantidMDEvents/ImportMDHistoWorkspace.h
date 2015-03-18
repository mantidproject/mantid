#ifndef MANTID_MDEVENTS_IMPORTMDHISTOWORKSPACE_H_
#define MANTID_MDEVENTS_IMPORTMDHISTOWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidMDEvents/ImportMDHistoWorkspaceBase.h"

namespace Mantid {
namespace MDEvents {

/** ImportMDHistoWorkspace : Takes a text file containing structured signal and
  error information and imports it
  as a new MDHistoWorkspace.

  @date 2012-06-20

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ImportMDHistoWorkspace : public ImportMDHistoWorkspaceBase {
public:
  ImportMDHistoWorkspace();
  virtual ~ImportMDHistoWorkspace();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Reads a text file and generates an MDHistoWorkspace from it.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
};

} // namespace MDEvents
} // namespace Mantid

#endif /* MANTID_MDEVENTS_IMPORTMDHISTOWORKSPACE_H_ */
