// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCOR_H_
#define MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCOR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** PolarizationEfficiencyCor: a generalised polarization correction
  algorithm. Depending on the value of property "CorrectionMethod" it
  calls either PolarizationCorrectionFredrikze or PolarizationCorrectionWildes
  inetrnally.
*/
class MANTID_ALGORITHMS_DLL PolarizationEfficiencyCor : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"PolarizationCorrectionFredrikze"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void execWildes();
  void execFredrikze();

  void checkWorkspaces() const;
  void checkWildesProperties() const;
  void checkFredrikzeProperties() const;

  std::vector<std::string> getWorkspaceNameList() const;
  API::WorkspaceGroup_sptr getWorkspaceGroup() const;
  API::MatrixWorkspace_sptr getEfficiencies();
  bool needInterpolation(API::MatrixWorkspace const &efficiencies,
                         API::MatrixWorkspace const &inWS) const;
  API::MatrixWorkspace_sptr
  convertToHistogram(API::MatrixWorkspace_sptr efficiencies);
  API::MatrixWorkspace_sptr interpolate(API::MatrixWorkspace_sptr efficiencies,
                                        API::MatrixWorkspace_sptr inWS);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCOR_H_ */
