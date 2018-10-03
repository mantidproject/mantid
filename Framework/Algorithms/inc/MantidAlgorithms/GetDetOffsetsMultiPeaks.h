// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_GetDetOffsetsMultiPeaks_H_
#define MANTID_ALGORITHMS_GetDetOffsetsMultiPeaks_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

struct FitPeakOffsetResult {
  double mask;
  double offset;
  double chi2;
  /// fit sum from GSL optimizer as offset's error
  double fitSum;
  /// summation of chi-square
  double chisqSum;
  /// Number of peaks with successful fitting
  double peakPosFittedSize;
  int numpeakstofit;
  int numpeaksfitted;
  int numpeaksindrange;
  std::string fitoffsetstatus;
  /// Highest peak position
  double highestpeakpos;
  /// Highest peak deviation after calibrated by offset
  double highestpeakdev;
  /// Average resolution delta(d)/d
  double resolution;
  /// Standard devation of the resolution
  double dev_resolution;
};

/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS
 @date 12/12/2011
 */
class DLLExport GetDetOffsetsMultiPeaks : public API::Algorithm {
public:
  /// Default constructorMatrix
  GetDetOffsetsMultiPeaks();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GetDetOffsetsMultiPeaks"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"GetDetectorOffsets"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\Calibration";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates an OffsetsWorkspace containing offsets for each detector. "
           "You can then save these to a .cal file using SaveCalFile.";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void processProperties();

  /// Create workspaces for fitting information
  void createInformationWorkspaces();

  /// Main function to calculate all detectors' offsets
  void calculateDetectorsOffsets();

  void
  importFitWindowTableWorkspace(DataObjects::TableWorkspace_sptr windowtablews);

  /// Call Gaussian as a Child Algorithm to fit the peak in a spectrum
  int fitSpectra(const int64_t wi, API::MatrixWorkspace_sptr inputW,
                 const std::vector<double> &peakPositions,
                 const std::vector<double> &fitWindows, size_t &nparams,
                 double &minD, double &maxD, std::vector<double> &peakPosToFit,
                 std::vector<double> &peakPosFitted, std::vector<double> &chisq,
                 std::vector<double> &peakHeights, int &i_highestpeak,
                 double &resolution, double &dev_resolution);

  /// Add peak fitting and offset calculation information to information table
  /// workspaces per spectrum
  void addInfoToReportWS(int wi, FitPeakOffsetResult offsetresult,
                         const std::vector<double> &tofitpeakpositions,
                         const std::vector<double> &fittedpeakpositions);

  /// Generate a list of peaks to calculate detectors' offset
  void generatePeaksList(const API::ITableWorkspace_sptr &peakslist, int wi,
                         const std::vector<double> &peakPositionRef,
                         std::vector<double> &peakPosToFit,
                         std::vector<double> &peakPosFitted,
                         std::vector<double> &peakHeightFitted,
                         std::vector<double> &chisq, bool useFitWindows,
                         const std::vector<double> &fitWindowsToUse,
                         const double minD, const double maxD,
                         double &deltaDovD, double &dev_deltaDovD);

  FitPeakOffsetResult
  calculatePeakOffset(const int wi, std::vector<double> &vec_peakPosFitted,
                      std::vector<double> &vec_peakPosRef);

  /// Calculate a spectrum's offset by optimizing offset
  void fitPeaksOffset(const size_t inpnparams, const double minD,
                      const double maxD,
                      const std::vector<double> &vec_peakPosRef,
                      const std::vector<double> &vec_peakPosFitted,
                      const std::vector<double> &vec_peakHeights,
                      FitPeakOffsetResult &fitresult);

  /// Make a summary on all fit
  void makeFitSummary();

  /// Remove rows without offset calculated from offset table workspace
  void removeEmptyRowsFromPeakOffsetTable();

  /// Input workspace
  API::MatrixWorkspace_sptr m_inputWS;
  /// Input EventWorkspace (from m_inputWS)
  DataObjects::EventWorkspace_const_sptr m_eventW;
  bool m_isEvent;

  /// Background type
  std::string m_backType;
  /// Peak profile type
  std::string m_peakType;
  /// Criterias for fitting peak
  std::string m_minimizer;
  double m_maxChiSq;
  double m_minPeakHeight;
  double m_leastMaxObsY;
  double m_maxOffset;

  std::vector<double> m_peakPositions;
  std::vector<double> m_fitWindows;

  /// Input resolution
  API::MatrixWorkspace_const_sptr m_inputResolutionWS;
  /// Flag of use input resolution
  bool m_hasInputResolution;
  /// Lower boundary of allowed peak width as resolution
  double m_minResFactor;
  /// Upper boundary of allowed peak width as resolution
  double m_maxResFactor;

  DataObjects::OffsetsWorkspace_sptr m_outputW;
  /// Output workspace for debugging purpose
  DataObjects::OffsetsWorkspace_sptr m_outputNP;
  /// Output Mask workspace
  API::MatrixWorkspace_sptr m_maskWS;

  DataObjects::TableWorkspace_sptr m_infoTableWS;
  DataObjects::TableWorkspace_sptr m_peakOffsetTableWS;
  /// Workspace for calculated detector resolution
  API::MatrixWorkspace_sptr m_resolutionWS;

  /// Flag to use fit window from TableWorkspace per spectrum
  bool m_useFitWindowTable;
  /// Vector of fit windows (also in vector)
  std::vector<std::vector<double>> m_vecFitWindow;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_GetDetOffsetsMultiPeaks_H_*/
