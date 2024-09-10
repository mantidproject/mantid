// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/DllConfig.h"
#include <memory>
#include <optional>

namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}
namespace MDAlgorithms {

DLLExport std::vector<double> gaussianKernel(const double fwhm);
DLLExport std::vector<double> normaliseKernel(std::vector<double> kernel);
DLLExport std::vector<double> renormaliseKernel(std::vector<double> kernel, const std::vector<bool> &validity);

/** SmoothMD : Algorithm for smoothing MDHistoWorkspaces
 */
class MANTID_MDALGORITHMS_DLL SmoothMD final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ThresholdMD"}; }
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

  std::shared_ptr<Mantid::API::IMDHistoWorkspace>
  hatSmooth(const std::shared_ptr<const Mantid::API::IMDHistoWorkspace> &toSmooth,
            const std::vector<double> &widthVector, const std::shared_ptr<Mantid::API::IMDHistoWorkspace> &weightingWS);

  std::shared_ptr<Mantid::API::IMDHistoWorkspace>
  gaussianSmooth(const std::shared_ptr<const Mantid::API::IMDHistoWorkspace> &toSmooth,
                 const std::vector<double> &widthVector,
                 const std::shared_ptr<Mantid::API::IMDHistoWorkspace> &weightingWS);

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid
