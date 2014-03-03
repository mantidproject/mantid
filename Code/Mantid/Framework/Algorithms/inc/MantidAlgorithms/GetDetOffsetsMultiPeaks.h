#ifndef MANTID_ALGORITHMS_GetDetOffsetsMultiPeaks_H_
#define MANTID_ALGORITHMS_GetDetOffsetsMultiPeaks_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_statistics.h>

namespace Mantid
{
namespace Algorithms
{

struct FitPeakOffsetResult
{
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
};

/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS
 @date 12/12/2011

 Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport GetDetOffsetsMultiPeaks: public API::Algorithm
{
public:
  /// Default constructorMatrix
  GetDetOffsetsMultiPeaks();
  /// Destructor
  virtual ~GetDetOffsetsMultiPeaks();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GetDetOffsetsMultiPeaks"; }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }
  /// Call Gaussian as a Child Algorithm to fit the peak in a spectrum
  int fitSpectra(const int64_t wi, API::MatrixWorkspace_sptr inputW, const std::vector<double> &m_peakPositions, const std::vector<double> &m_fitWindows, size_t &nparams,
                  double &minD, double &maxD,
                  std::vector<double>&peakPosToFit, std::vector<double> &peakPosFitted, std::vector<double> &chisq,
                 int &i_highestpeak);

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();

  void processProperties();

  void addInfoToReportWS(int wi, FitPeakOffsetResult offsetresult, const std::vector<double> &tofitpeakpositions,
                         const std::vector<double> &fittedpeakpositions);

  void generatePeaksList(const API::ITableWorkspace_sptr &peakslist,
                         int wi,
                         const std::vector<double> &peakPositionRef,
                         std::vector<double> &peakPosToFit,
                         std::vector<double> &peakPosFitted,
                         std::vector<double> &peakHeightFitted, std::vector<double> &chisq, bool useFitWindows,
                         const std::vector<double> &fitWindowsToUse, const double minD, const double maxD);

  /// Generate output information table workspace
  Mantid::DataObjects::TableWorkspace_sptr createOutputInfoTable(size_t numspec);

  /// Generate output peak information table workspace
  Mantid::DataObjects::TableWorkspace_sptr createOutputPeakOffsetTable(size_t numspec);

  FitPeakOffsetResult calculatePeakOffset(const int wi, std::vector<double>& fittedpeakpositions, std::vector<double>& vec_peakPosRef);

  void fitPeaksOffset(const size_t inpnparams, const double minD, const double maxD,
                      const std::vector<double>& vec_peakPosRef,
                      const std::vector<double>& vec_peakPosFitted,
                      const std::vector<double>& vec_fitChi2,
                      FitPeakOffsetResult& fitresult);

  void makeFitSummary();

  void removeEmptyRowsFromPeakOffsetTable();

  API::MatrixWorkspace_sptr inputW;
  DataObjects::EventWorkspace_const_sptr eventW;
  bool isEvent;

  std::string m_backType;
  std::string m_peakType;
  double m_maxChiSq;
  double m_minPeakHeight;

  double maxOffset;

  std::vector<double> m_peakPositions;
  std::vector<double> m_fitWindows;

  DataObjects::TableWorkspace_sptr m_infoTableWS;
  DataObjects::TableWorkspace_sptr m_peakOffsetTableWS;

  DataObjects::OffsetsWorkspace_sptr outputW;
  DataObjects::OffsetsWorkspace_sptr outputNP;
  API::MatrixWorkspace_sptr maskWS;

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_GetDetOffsetsMultiPeaks_H_*/
