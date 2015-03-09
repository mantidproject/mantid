#ifndef MANTID_MDALGORITHMS_CONVERTTOMDMINMAX_LOCAL_H_
#define MANTID_MDALGORITHMS_CONVERTTOMDMINMAX_LOCAL_H_

#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/ConvertToMDParent.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertToMDMinMaxLocal : Algorithm to calculate limits for ConvertToMD

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ConvertToMDMinMaxLocal : public ConvertToMDParent {
public:
  ConvertToMDMinMaxLocal();
  virtual ~ConvertToMDMinMaxLocal();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculate limits of ConvertToMD transformation possible for this "
           "particular workspace and the instrument, attached to it.";
  }

  virtual int version() const { return 1; }

protected: // for testing
  void findMinMaxValues(MDWSDescription &targWSDescr,
                        MDTransfInterface *const qTransf,
                        Kernel::DeltaEMode::Type dEMode,
                        std::vector<double> &MinValues,
                        std::vector<double> &MaxValues);

private:
  void exec();
  void init();
  /// pointer to the input workspace;
  Mantid::DataObjects::Workspace2D_sptr m_MinMaxWS2D;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTTOMDHELPER_H_ */
