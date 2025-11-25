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
#include "MantidDataObjects/TableWorkspace.h"

#include <boost/container/flat_set.hpp>
#include <limits>

namespace Mantid {
namespace Crystal {

/** SCDCalibratePanels2 :
 * Using input peakworkspace with indexation results to calibrate each
 * individual panels.
 * The target calibration properties include:
 * - T0: micro seconds
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

class MANTID_CRYSTAL_DLL SCDCalibratePanels2 : public Mantid::API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SCDCalibratePanels"; }

  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Panel parameters and L1 are optimized to "
           "minimize errors between theoretical and actual q values for the "
           "peaks";
  }

  /// Algorithm's version, overriding a virtual method
  int version() const override { return 2; }

  /// Algorithm's category, overriding a virtual method
  const std::string category() const override { return "Crystal\\Corrections"; }

  /// Extra help info
  const std::vector<std::string> seeAlso() const override { return {"CalculateUMatrix"}; }

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// Private validator for inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Cache TOF equivalent to those measured from experiment
  std::vector<double> captureTOF(const Mantid::API::IPeaksWorkspace_sptr &pws);

  /// Private function dedicated for parsing lattice constant
  void parseLatticeConstant(const Mantid::API::IPeaksWorkspace_sptr &pws);

  /// Update the UB matrix
  void updateUBMatrix(const Mantid::API::IPeaksWorkspace_sptr &pws);

  /// Remove unindexed peaks from workspace
  Mantid::API::IPeaksWorkspace_sptr removeUnindexedPeaks(const Mantid::API::IPeaksWorkspace_sptr &pws);

  /// Private function for getting names of banks to be calibrated
  void getBankNames(const Mantid::API::IPeaksWorkspace_sptr &pws);

  /// Private function for calibrating T0
  void optimizeT0(Mantid::API::IPeaksWorkspace_sptr pws, Mantid::API::IPeaksWorkspace_sptr pws_original);

  /// Private function for calibrating L1
  void optimizeL1(Mantid::API::IPeaksWorkspace_sptr pws, Mantid::API::IPeaksWorkspace_sptr pws_original);

  /// Private function for calibrating banks
  void optimizeBanks(Mantid::API::IPeaksWorkspace_sptr pws, const Mantid::API::IPeaksWorkspace_sptr &pws_original,
                     const bool &docalibsize, const double &sizesearchradius, const bool &fixdetxyratio);

  /// Private function for fine tunning sample position
  void optimizeSamplePos(Mantid::API::IPeaksWorkspace_sptr pws, Mantid::API::IPeaksWorkspace_sptr pws_original);

  /// Helper function for selecting peaks based on given bank name
  Mantid::API::IPeaksWorkspace_sptr selectPeaksByBankName(const Mantid::API::IPeaksWorkspace_sptr &pws,
                                                          const std::string &bankname, const std::string &outputwsn);

  /// Helper function that calculates the ideal qSample based on
  /// integer HKL
  Mantid::API::MatrixWorkspace_sptr getIdealQSampleAsHistogram1D(const Mantid::API::IPeaksWorkspace_sptr &pws);

  /// Helper functions for adjusting components
  void adjustComponent(double dx, double dy, double dz, double drx, double dry, double drz, double scalex,
                       double scaley, const std::string &cmptName, Mantid::API::IPeaksWorkspace_sptr &pws);

  /// Generate a Table workspace to store the calibration results
  Mantid::API::ITableWorkspace_sptr generateCalibrationTable(std::shared_ptr<Geometry::Instrument> &instrument,
                                                             const Geometry::ParameterMap &pmap);

  /// Save to xml file for Mantid to load by manual crafting
  void saveXmlFile(const std::string &FileName, const boost::container::flat_set<std::string> &AllBankNames,
                   std::shared_ptr<Geometry::Instrument> &instrument, const Geometry::ParameterMap &pmap);

  /// Save to ISAW type det calibration output for backward compatiblity
  void saveIsawDetCal(const std::string &filename, boost::container::flat_set<std::string> &AllBankName,
                      std::shared_ptr<Geometry::Instrument> &instrument, double T0);

  /// Save the calibration table to a CSV file
  void saveCalibrationTable(const std::string &FileName, Mantid::API::ITableWorkspace_sptr const &tws);

  /// Profile related functions
  void profileL1(Mantid::API::IPeaksWorkspace_sptr &pws, Mantid::API::IPeaksWorkspace_sptr pws_original);
  void profileBanks(Mantid::API::IPeaksWorkspace_sptr const &pws,
                    const Mantid::API::IPeaksWorkspace_sptr &pws_original);
  void profileT0(Mantid::API::IPeaksWorkspace_sptr &pws, Mantid::API::IPeaksWorkspace_sptr pws_original);
  void profileL1T0(Mantid::API::IPeaksWorkspace_sptr &pws, Mantid::API::IPeaksWorkspace_sptr pws_original);

  /// Retrieve "scalex" and "scaley" from a workspace's parameter map if the component is rectangular detector
  std::pair<double, double> getRectangularDetectorScaleFactors(std::shared_ptr<Geometry::Instrument> &instrument,
                                                               const std::string &bankname,
                                                               const Geometry::ParameterMap &pmap);

  /// unique vars for a given instance of calibration
  double m_a = 0.0, m_b = 0.0, m_c = 0.0, m_alpha = 0.0, m_beta = 0.0, m_gamma = 0.0, m_T0 = 0.0;
  bool LOGCHILDALG{true};
  int maxFitIterations{500};
  const int MINIMUM_PEAKS_PER_BANK{6};
  std::string mCalibBankName{""};
  const double PI{3.1415926535897932384626433832795028841971693993751058209};
  static constexpr double Tolerance = std::numeric_limits<double>::epsilon();

  // Column names and types
  const std::vector<std::string> calibrationTableColumnNames{
      "ComponentName",    "Xposition",        "Yposition",     "Zposition", "XdirectionCosine",
      "YdirectionCosine", "ZdirectionCosine", "RotationAngle", "ScaleX",    "ScaleY"};
  const std::vector<std::string> calibrationTableColumnTypes{"str",    "double", "double", "double", "double",
                                                             "double", "double", "double", "double", "double"};

  boost::container::flat_set<std::string> m_BankNames;
  Mantid::API::ITableWorkspace_sptr mCaliTable;
};

} // namespace Crystal
} // namespace Mantid
