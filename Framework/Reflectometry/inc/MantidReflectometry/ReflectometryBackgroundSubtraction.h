// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {

/** ReflectometryBackgroundSubtraction : This is an algorithm that computes the
 * background of a given workspace and removes it from the input workspace.
 */
class MANTID_REFLECTOMETRY_DLL ReflectometryBackgroundSubtraction
    : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void
  calculateAverageSpectrumBackground(const API::MatrixWorkspace_sptr &inputWS,
                                     const std::vector<specnum_t> &spectraList);
  void calculatePolynomialBackground(API::MatrixWorkspace_sptr inputWS,
                                     const std::vector<double> &spectrumRanges);
  std::vector<double>
  findSpectrumRanges(const std::vector<specnum_t> &spectraList);
  void calculatePixelBackground(const API::MatrixWorkspace_sptr &inputWS,
                                const std::vector<double> &indexRanges);

  /** Overridden Algorithm methods **/

  // Initialize the algorithm
  void init() override;
  // Execute the algorithm
  void exec() override;
  // validate the inputs
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace Reflectometry
} // namespace Mantid
