// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {

/** Save a PeaksWorkspace to a Gsas-style ASCII .hkl file.
 *
 * @author Vickie Lynch, SNS
 * @date 2011-09-28
 */

class MANTID_CRYSTAL_DLL SaveHKL final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveHKL"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Save a PeaksWorkspace to a ASCII .hkl file."; }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadHKL", "SaveHKLCW"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\DataHandling;DataHandling\\Text"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  double absorbSphere(double radius, double twoth, double wl, double &tbar);
  double spectrumCalc(double TOF, int iSpec, std::vector<std::vector<double>> time,
                      std::vector<std::vector<double>> spectra, size_t id);
  void sizeBanks(const std::string &bankName, int &nCols, int &nRows);

  DataObjects::PeaksWorkspace_sptr m_ws;
  double m_smu = 0.0; // in 1/cm
  double m_amu = 0.0; // in 1/cm
  double m_power_th = 0.0;
};

} // namespace Crystal
} // namespace Mantid
