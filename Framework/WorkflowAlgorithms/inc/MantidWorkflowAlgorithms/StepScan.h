#ifndef MANTID_WORKFLOWALGORITHMS_ROCKINGCURVE_H_
#define MANTID_WORKFLOWALGORITHMS_ROCKINGCURVE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/** This workflow algorithm is for generation of a rocking curve from an
   alignment scan performed
    on an ADARA-enabled instrument at the SNS.
    An important thing to note about this algorithm is that it may modify the
   input workspace.

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
class DLLExport StepScan : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Workflow algorithm for analysis of an alignment scan from an SNS "
           "Adara-enabled beam line";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  DataObjects::EventWorkspace_sptr
  getMonitorWorkspace(API::MatrixWorkspace_sptr inputWS);
  DataObjects::EventWorkspace_sptr
  cloneInputWorkspace(API::Workspace_sptr inputWS);
  void runMaskDetectors(API::MatrixWorkspace_sptr inputWS,
                        API::MatrixWorkspace_sptr maskWS);
  void runFilterByXValue(API::MatrixWorkspace_sptr inputWS, const double xmin,
                         const double xmax);
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_ROCKINGCURVE_H_ */
