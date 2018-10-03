// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_SMOOTHMD_H_
#define MANTID_MDALGORITHMS_SMOOTHMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}
namespace MDAlgorithms {

DLLExport std::vector<double> gaussianKernel(const double fwhm);
DLLExport std::vector<double> normaliseKernel(std::vector<double> kernel);
DLLExport std::vector<double>
renormaliseKernel(std::vector<double> kernel,
                  const std::vector<bool> &validity);

/** SmoothMD : Algorithm for smoothing MDHistoWorkspaces
*/
class DLLExport SmoothMD : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ThresholdMD"};
  }
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;

  boost::shared_ptr<Mantid::API::IMDHistoWorkspace> hatSmooth(
      boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> toSmooth,
      const std::vector<double> &widthVector,
      boost::optional<boost::shared_ptr<const Mantid::API::IMDHistoWorkspace>>
          weightingWS);

  boost::shared_ptr<Mantid::API::IMDHistoWorkspace> gaussianSmooth(
      boost::shared_ptr<const Mantid::API::IMDHistoWorkspace> toSmooth,
      const std::vector<double> &widthVector,
      boost::optional<boost::shared_ptr<const Mantid::API::IMDHistoWorkspace>>
          weightingWS);

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_SMOOTHMD_H_ */
