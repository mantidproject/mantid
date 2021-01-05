// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/PeaksWorkspace.h"

#include <boost/container/flat_set.hpp>

namespace Mantid {
namespace Crystal {

/** SCDCalibratePanels2 :
 * Using input peakworkspace with indexation results to calibrate each
 * individual panels.
 * The target calibration properties include:
 * - T0: sec?
 *       time for proton to travel from reactor to target to generate neutron 
 * - L1: meters
 *       distance between target and sample
 * - L2: meters (also known as z_shift)
 *       distance between sample and the center of each panel
 * - Rot: degrees
 *       Euler angles (xyz )
 * 
 * Spirit successor of ISAW and its reincarnation: SCDCalibratePanels
*/
class MANTID_CRYSTAL_DLL SCDCalibratePanels2 : public Mantid::API::Algorithm{
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SCDCalibratePanels"; }

  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Panel parameters and L0 are optimized to "
           "minimize errors between theoretical and actual q values for the "
           "peaks";
  }

  /// Algorithm's version, overriding a virtual method
  int version() const override { return 2; }

  /// Algorithm's category, overriding a virtual method
  const std::string category() const override {
    return "Crystal\\Corrections";
  }

  /// Extra help info
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateUMatrix"};
  }

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// Private validator for inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Private function dedicated for parsing lattice constant
  void parseLatticeConstant(std::shared_ptr<Mantid::DataObjects::PeaksWorkspace> pws);

  /// Private function for getting names of banks to be calibrated
  void getBankNames(std::shared_ptr<Mantid::DataObjects::PeaksWorkspace> pws);

  /// Private function for calibrating T0
  void optimizeT0(std::shared_ptr<Mantid::DataObjects::PeaksWorkspace> pws);

  /// Private function for calibrating L1
  void optimizeL1(std::shared_ptr<Mantid::DataObjects::PeaksWorkspace> pws);

  /// Private function for calibrating banks
  void optimizeBanks(std::shared_ptr<Mantid::DataObjects::PeaksWorkspace> pws);

  /// Helper functions for adjusting components
  void adjustComponent(double dx, double dy, double dz, double rvx, double rvy,
                       double rvz, double rang, std::string cmptName,
                       DataObjects::PeaksWorkspace_sptr &pws);

  /// Save to xml file for Mantid to load
  void saveXmlFile(const std::string &FileName,
                   boost::container::flat_set<std::string> &AllBankNames,
                   std::shared_ptr<Geometry::Instrument> &instrument);

  /// Save to ISAW type det calibration output for backward compatiblity
  void saveIsawDetCal(const std::string &filename,
                      boost::container::flat_set<std::string> &AllBankName,
                      std::shared_ptr<Geometry::Instrument> &instrument,
                      double T0);

  /// unique vars for a given instance of calibration
  double m_a, m_b, m_c, m_alpha, m_beta, m_gamma;
  double m_T0 = 0.0;
  double m_L1 = 2000.0;
  double m_tolerance_translation = 1e-4; // meters
  double m_tolerance_rotation = 1e-3;    // degree
  // The following bounds are set based on information provided by the
  // CORELLI team
  double m_bank_translation_bounds = 5e-2;  // meter
  double m_bank_rotation_bounds = 5;        // degree
  double m_source_translation_bounds = 0.1; // meter
  boost::container::flat_set<std::string> m_BankNames;
  const bool LOGCHILDALG{false};
  const int MINIMUM_PEAKS_PER_BANK{6};
  const double PI{3.141592653589793238462643383279502884};
};

} // namespace Crystal
} // namespace Mantid
