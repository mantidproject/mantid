#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"
#include <boost/optional.hpp>

namespace Mantid {
namespace Algorithms {

/** CreateTransmissionWorkspaceAuto : Creates a transmission run workspace in
Wavelength from input TOF workspaces.

Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CreateTransmissionWorkspaceAuto
    : public API::DataProcessorAlgorithm {
public:
  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override {
    return "CreateTransmissionWorkspaceAuto";
  }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Reflectometry\\ISIS"; }
  /// Algorithm's summary for documentation
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  template <typename T> boost::optional<T> isSet(std::string propName) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACEAUTO_H_ */
