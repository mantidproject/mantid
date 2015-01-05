#ifndef MANTID_MDALGORITHMS_WEIGHTEDMEANMD_H_
#define MANTID_MDALGORITHMS_WEIGHTEDMEANMD_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** WeightedMeanMD : Find the weighted mean of two MDWorkspaces

  @date 2012-06-26

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
class DLLExport WeightedMeanMD : public BinaryOperationMD {
public:
  WeightedMeanMD();
  virtual ~WeightedMeanMD();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "WeightedMeanMD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Find weighted mean of two MDHistoWorkspaces.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };

private:
  /// Is the operation commutative?
  bool commutative() const;

  /// Check the inputs and throw if the algorithm cannot be run
  void checkInputs();

  /// Run the algorithm with an MDEventWorkspace as output
  void execEvent();

  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out,
                      Mantid::MDEvents::MDHistoWorkspace_const_sptr operand);

  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void
  execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr out,
                  Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_WEIGHTEDMEANMD_H_ */