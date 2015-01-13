#ifndef MANTID_MDEVENTS_QUERYMDWORKSPACE_H_
#define MANTID_MDEVENTS_QUERYMDWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MDEventWorkspace.h"

namespace Mantid {
namespace MDEvents {

/** QueryMDWorkspace : Query an MDWorkspace in order to extract overview
  information as a table workspace. Signal and Error squared values as well as
  extent information are exported.

  @date 2011-11-22

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport QueryMDWorkspace : public Mantid::API::Algorithm {
public:
  QueryMDWorkspace();
  virtual ~QueryMDWorkspace();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "QueryMDWorkspace"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Query the IMDWorkspace in order to extract summary information.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  template <typename MDE, size_t nd>
  void
  getBoxData(typename Mantid::MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);
};

} // namespace MDEvents
} // namespace Mantid

#endif /* MANTID_MDEVENTS_QUERYMDWORKSPACE_H_ */
