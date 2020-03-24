// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {

/**
 Algorithm to create a flood correction workspace for reflectometry
 data reduction.
 */
class MANTID_REFLECTOMETRY_DLL CreateFloodWorkspace : public API::Algorithm {
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
  API::MatrixWorkspace_sptr integrate(const API::MatrixWorkspace_sptr &ws);
  API::MatrixWorkspace_sptr transpose(const API::MatrixWorkspace_sptr &ws);
  bool shouldRemoveBackground();
  void collectExcludedSpectra();
  bool isExcludedSpectrum(double spec) const;
  API::MatrixWorkspace_sptr removeBackground(API::MatrixWorkspace_sptr ws);
  API::MatrixWorkspace_sptr scaleToCentralPixel(API::MatrixWorkspace_sptr ws);

  std::vector<double> m_excludedSpectra;
};

} // namespace Reflectometry
} // namespace Mantid
