#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAlgorithms/ReflectometryWorkflowBase.h"

namespace Mantid {
namespace Algorithms {

/** CreateTransmissionWorkspace : Create a transmission run workspace in
 Wavelength given one or more TOF workspaces

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
class DLLExport CreateTransmissionWorkspace : public ReflectometryWorkflowBase {
public:
  CreateTransmissionWorkspace();
  virtual ~CreateTransmissionWorkspace();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Creates a transmission run workspace in Wavelength from input TOF "
           "workspaces.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  /// Make a transmission correction workspace
  API::MatrixWorkspace_sptr makeTransmissionCorrection(
      const std::string &processingCommands, const MinMax &wavelengthInterval,
      const MinMax &wavelengthMonitorBackgroundInterval,
      const MinMax &wavelengthMonitorIntegrationInterval,
      const int &i0MonitorIndex, API::MatrixWorkspace_sptr firstTransmissionRun,
      OptionalMatrixWorkspace_sptr secondTransmissionRun,
      const OptionalDouble &stitchingStartQ,
      const OptionalDouble &stitchingDeltaQ,
      const OptionalDouble &stitchingEndQ,
      const OptionalDouble &stitchingStartOverlapQ,
      const OptionalDouble &stitchingEndOverlapQ, const double &wavelengthStep);

  void init();
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE_H_ */
