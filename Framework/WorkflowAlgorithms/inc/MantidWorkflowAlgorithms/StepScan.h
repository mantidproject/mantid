// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
