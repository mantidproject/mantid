// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CREATEFLOODWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATEFLOODWORKSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {

/**
 Algorithm to create a flood correction workspace for reflectometry
 data reduction.
 */
class DLLExport CreateFloodWorkspace : public API::Algorithm {
public:
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  API::MatrixWorkspace_sptr getInputWorkspace();
  std::string getBackgroundFunction();
  API::MatrixWorkspace_sptr integrate(API::MatrixWorkspace_sptr ws);
  API::MatrixWorkspace_sptr transpose(API::MatrixWorkspace_sptr ws);
  bool shouldRemoveBackground();
  void collectExcludedSpectra();
  bool isExcludedSpectrum(double spec) const;
  API::MatrixWorkspace_sptr removeBackground(API::MatrixWorkspace_sptr ws);
  API::MatrixWorkspace_sptr scaleToCentralPixel(API::MatrixWorkspace_sptr ws);

  std::vector<double> m_excludedSpectra;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATEFLOODWORKSPACE_H_ */
