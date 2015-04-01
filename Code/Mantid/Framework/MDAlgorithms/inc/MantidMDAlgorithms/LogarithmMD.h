#ifndef MANTID_MDALGORITHMS_LOGARITHMMD_H_
#define MANTID_MDALGORITHMS_LOGARITHMMD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/UnaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** LogarithmMD : natural logarithm of a MDHistoWorkspace

  @date 2011-11-08

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
class DLLExport LogarithmMD : public UnaryOperationMD {
public:
  LogarithmMD();
  virtual ~LogarithmMD();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Perform a natural logarithm of a MDHistoWorkspace.";
  }

  virtual int version() const;

private:
  virtual void initExtraProperties();

  /// Check the inputs and throw if the algorithm cannot be run
  void checkInputs();

  /// Run the algorithm on a MDEventWorkspace
  void execEvent(Mantid::API::IMDEventWorkspace_sptr out);

  /// Run the algorithm with a MDHistoWorkspace
  void execHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_LOGARITHMMD_H_ */
