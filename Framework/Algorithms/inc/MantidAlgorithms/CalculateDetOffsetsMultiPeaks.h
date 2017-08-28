#ifndef MANTID_ALGORITHMS_CalculateDetOffsetsMultiPeaks_H_
#define MANTID_ALGORITHMS_CalculateDetOffsetsMultiPeaks_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

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

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport CalculateDetOffsetsMultiPeaks : public API::Algorithm {
public:
  /// Default constructorMatrix
  CalculateDetOffsetsMultiPeaks();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CalculateDetOffsetsMultiPeaks"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
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

  /// print a vector
  void printVector(const std::string &title,
                   const std::vector<double> &vec2print);

  /// Input workspace
  API::MatrixWorkspace_sptr m_inputWS;
  /// Input EventWorkspace (from m_inputWS)
  DataObjects::EventWorkspace_const_sptr m_eventW;
  bool m_isEvent;

  /// Criterias for fitting peak
  // std::string m_minimizer;
  double m_maxChiSq;

  double m_maxOffset;

  std::vector<double> m_peakPositions;

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
  /// reference workspace containing instrument
  API::MatrixWorkspace_const_sptr m_refWorkspace;

  /// Flag to use fit window from TableWorkspace per spectrum
  bool m_useFitWindowTable;
  /// Vector of fit windows (also in vector)
  std::vector<std::vector<double>> m_vecFitWindow;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_CalculateDetOffsetsMultiPeaks_H_*/
