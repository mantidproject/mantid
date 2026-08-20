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
namespace Geometry {
class ComponentInfo;
}

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

  /** Slant path length through the detector, derived from the calibrated sample-to-panel distance.
   *
   * Static and public so that it can be asserted directly. An end-to-end test cannot isolate this term,
   * because displacing a panel also changes L2 and the scattering angle, which move the correction
   * through other factors whether or not the distance is taken from calibrated geometry.
   */
  static double slantPathLength(const Geometry::Instrument_const_sptr &inst,
                                const Geometry::ComponentInfo &componentInfo, const std::string &bankName,
                                const double L2, const double depth);

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  double absorbSphere(double radius, double twoth, double wl, double &tbar);
  double spectrumCalc(double TOF, int iSpec, const std::vector<std::vector<double>> &time,
                      const std::vector<std::vector<double>> &spectra, size_t id);
  void sizeBanks(const std::string &bankName, int &nCols, int &nRows);

  DataObjects::PeaksWorkspace_sptr m_ws;
  double m_smu = 0.0; // in 1/cm
  double m_amu = 0.0; // in 1/cm
  double m_power_th = 0.0;
};

} // namespace Crystal
} // namespace Mantid
