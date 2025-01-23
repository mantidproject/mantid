// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

class RemovePeaks {
public:
  void setup(const DataObjects::TableWorkspace_sptr &peaktablews);

  DataObjects::Workspace2D_sptr removePeaks(const API::MatrixWorkspace_const_sptr &dataws, int wsindex, double numfwhm);

private:
  /// Parse peak centre and FWHM from a table workspace
  void parsePeakTableWorkspace(const DataObjects::TableWorkspace_sptr &peaktablews, std::vector<double> &vec_peakcentre,
                               std::vector<double> &vec_peakfwhm);

  /// Exclude peak regions
  size_t excludePeaks(std::vector<double> v_inX, std::vector<bool> &v_useX, const std::vector<double> &v_centre,
                      const std::vector<double> &v_fwhm, double num_fwhm);

  std::vector<double> m_vecPeakCentre;
  std::vector<double> m_vecPeakFWHM;
};

/** ProcessBackground : Process background obtained from LeBailFit
 */
class MANTID_CURVEFITTING_DLL ProcessBackground final : public API::Algorithm {
public:
  ProcessBackground();

  const std::string category() const override { return "Diffraction\\Utility"; }

  const std::string name() const override { return "ProcessBackground"; }

  int version() const override { return 1; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "ProcessBackground provides some tools to process powder "
           "diffraction pattern's "
           "background in order to help Le Bail Fit.";
  }

private:
  /// Define properties
  void init() override;

  /// Execution body
  void exec() override;

  /// Set up dummy output optional workspaces
  void setupDummyOutputWSes();

  /// Select b...
  void selectBkgdPoints();

  /// Select background points (main)
  void selectFromGivenXValues();

  /// Select background points (main)
  void selectFromGivenFunction();

  /// Select background points automatically
  DataObjects::Workspace2D_sptr autoBackgroundSelection(const DataObjects::Workspace2D_sptr &bkgdWS);

  /// Create a background function from input properties
  BackgroundFunction_sptr createBackgroundFunction(const std::string &backgroundtype);

  /// Filter non-background data points out and create a background workspace
  DataObjects::Workspace2D_sptr filterForBackground(const BackgroundFunction_sptr &bkgdfunction);

  DataObjects::Workspace2D_const_sptr m_dataWS;
  DataObjects::Workspace2D_sptr m_outputWS;

  int m_wsIndex;

  double m_lowerBound;
  double m_upperBound;

  std::string m_bkgdType;

  // bool m_doFitBackground;

  // double mTolerance;

  /// Number of FWHM of range of peak to be removed.
  double m_numFWHM;

  /// Remove peaks in a certain region
  void removePeaks();

  /// Remove a certain region from input workspace
  void deleteRegion();

  /// Add a certain region from a reference workspace
  void addRegion();

  void fitBackgroundFunction(const std::string &bkgdfunctiontype);
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
