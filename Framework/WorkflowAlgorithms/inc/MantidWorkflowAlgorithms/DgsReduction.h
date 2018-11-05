// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_WORKFLOWALGORITHMS_DGSREDUCTION_H_
#define MANTID_WORKFLOWALGORITHMS_DGSREDUCTION_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** DgsReduction : This is the top-level workflow algorithm for controlling
 * direct geometry spectrometer reduction.

@date 2012-06-06
 */
class DLLExport DgsReduction : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Top-level workflow algorithm for DGS reduction.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  API::Workspace_sptr loadInputData(const std::string prop,
                                    const bool mustLoad = true);
  API::MatrixWorkspace_sptr loadGroupingFile(const std::string prop);
  API::MatrixWorkspace_sptr loadHardMask();
  double getParameter(std::string algParam, API::MatrixWorkspace_sptr ws,
                      std::string altParam);

  boost::shared_ptr<Kernel::PropertyManager> reductionManager;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_DGSREDUCTION_H_ */
