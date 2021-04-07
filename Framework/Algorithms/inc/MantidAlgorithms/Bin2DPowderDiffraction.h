// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** Bin2DPowderDiffraction :

  This algorithms performs binning of TOF powder diffraction event data
  in 2D d-Spacing (d, d_perp) as described in
  J. Appl. Cryst. (2015) 48, 1627-1636 and
  J. Appl. Cryst. (2017) 50, 866-875
*/
class MANTID_ALGORITHMS_DLL Bin2DPowderDiffraction : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"Rebin2D"}; }
  const std::string category() const override;
  const std::string summary() const override;
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

protected:
  std::shared_ptr<API::Progress> m_progress;

private:
  void init() override;
  void exec() override;
  /// Setup the output workspace
  API::MatrixWorkspace_sptr createOutputWorkspace();
  void ReadBinsFromFile(std::vector<double> &Ybins, std::vector<std::vector<double>> &Xbins) const;
  size_t UnifyXBins(std::vector<std::vector<double>> &Xbins) const;

  DataObjects::EventWorkspace_sptr m_inputWS; ///< Pointer to the input event workspace
  int m_numberOfSpectra;                      ///< The number of spectra in the workspace
  void normalizeToBinArea(const API::MatrixWorkspace_sptr &outWS);
};

double calcD(double wavelength, double sintheta);
double calcDPerp(double wavelength, double logcostheta);

} // namespace Algorithms
} // namespace Mantid
