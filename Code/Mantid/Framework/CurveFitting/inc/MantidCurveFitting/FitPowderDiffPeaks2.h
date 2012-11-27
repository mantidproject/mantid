#ifndef MANTID_CURVEFITTING_FITPOWDERDIFFPEAKS2_H_
#define MANTID_CURVEFITTING_FITPOWDERDIFFPEAKS2_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidCurveFitting/Polynomial.h"
#include "MantidCurveFitting/BackToBackExponential.h"

using namespace std;
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

namespace Mantid
{
namespace CurveFitting
{

  /** FitPowderDiffPeaks2 : Fit peaks in powder diffraction pattern.

    Its application is to serve as the first step for refining powder diffractomer instrument
    parameters. Its output will be used by RefinePowderInstrumentParameters().
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport FitPowderDiffPeaks2 : public API::Algorithm
  {
  public:
    FitPowderDiffPeaks2();
    virtual ~FitPowderDiffPeaks2();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "FitPowderDiffPeaks2";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Implement abstract Algorithm methods
    void init();
    /// Implement abstract Algorithm methods
    void exec();

    /// Generate peaks from input table workspace
    void genPeaksFromTable(DataObjects::TableWorkspace_sptr peakparamws);

    /// Generate a peak
    BackToBackExponential_sptr genPeak(map<string, int> hklmap, map<string, double> parammap,
                                       map<string, string> bk2bk2braggmap, bool &good, vector<int> &hkl, double &d_h);

    /// Get (HKL) from a map; Return false if the information is incomplete
    bool getHKLFromMap(map<string, int> intmap, vector<int>& hkl);

    /// Import instrument parameters from (input) table workspace
    void importInstrumentParameterFromTable(DataObjects::TableWorkspace_sptr parameterWS);

    /// Import Bragg peak table workspace
    void parseBraggPeakTable(DataObjects::TableWorkspace_sptr peakws, vector<map<string, double> >& parammaps,
                             vector<map<string, int> >& hklmaps);

    /// Fit peaks
    void fitPeaksTrustInput();

    /// Fit peaks in robust algorithm
    void fitPeaksRobust();

    /// Fit a single peak
    bool fitPeak(BackToBackExponential_sptr peak, BackgroundFunction_sptr background, double leftdev,
                 double rightdev, size_t m_wsIndex, double &chi2);

    //---------------------------------------------------------------------------

    /// Fit single peak in robust mode (no hint)
    bool fitSinglePeakRobust(BackToBackExponential_sptr peak, BackgroundFunction_sptr background,
                             double leftdev, double rightdev, double& chi2);

    /// Fit non-right-most single peak in robust mode.  Estimation from right peak
    bool fitSinglePeakRefRight(BackToBackExponential_sptr peak, BackgroundFunction_sptr backgroundfunction,
                               BackToBackExponential_sptr rightpeak, double peakleftbound, double peakrightbound,
                               double& chi2);



    /// Fit peak with confidence of the centre
    bool fitSinglePeakConfident(BackToBackExponential_sptr peak);

    /// Fit peak with trustful peak parameters
    bool fitPeakConfident(DataObjects::Workspace2D_sptr dataws, BackToBackExponential_sptr peak,
                          BackgroundFunction_sptr backgroundfunction);

    /// Fit peak with confident parameters
    bool fitPeakConfident(Workspace2D_sptr dataws, BackToBackExponential_sptr peak, double dampingfactor);

    /// Fit peaks with confidence in fwhm and etc.
    bool fitOverlappedPeaks(vector<BackToBackExponential_sptr> peaks, double gfwhm);

    //---------------------------------------------------------------------------

    /// Find max height (peak center)
    bool findMaxHeight(API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax, double& center, double& centerleftbound, double& centerrightbound, int &errordirection);

    /// Generate output peak parameters workspace
    std::pair<DataObjects::TableWorkspace_sptr, DataObjects::TableWorkspace_sptr> genPeakParametersWorkspace(
        std::vector<size_t> goodfitpeaks, std::vector<double> goodfitchi2s);

    /// Crop data workspace
    void cropWorkspace(double tofmin, double tofmax);

    /// Parse Fit() output parameter workspace
    std::string parseFitParameterWorkspace(API::ITableWorkspace_sptr paramws);

    /// Build a partial workspace from source
    // DataObjects::Workspace2D_sptr buildPartialWorkspace(API::MatrixWorkspace_sptr sourcews, size_t m_wsIndex,
    // double leftbound, double rightbound);

    /// Estimate the range of a single peak
    bool estimateSinglePeakRange(BackToBackExponential_sptr peak, BackgroundFunction_sptr background,
                                 BackToBackExponential_sptr rightpeak, double fwhm, bool ismostright,
                                                     size_t m_wsIndex, double& chi2);

    /// Estimate background
    // void estimateBackground(DataObjects::Workspace2D_sptr dataws);

    /// Subtract background (This is an operation within the specially defined data workspace for peak fitting)
    void subtractBackground(DataObjects::Workspace2D_sptr dataws);

    /// Estimate FWHM for the peak observed
    bool estimateFWHM(DataObjects::Workspace2D_sptr dataws, size_t wsindex, double tof_h, double& leftfwhm, double& rightfwhm);

    /// Fit background function by removing the peak properly
    bool doFitBackground(DataObjects::Workspace2D_sptr dataws, BackgroundFunction_sptr background,
                         double leftpeakbound, double rightpeakbound);

    /// Fit single peak without background
    std::pair<bool, double> doFitPeak(Workspace2D_sptr dataws, BackToBackExponential_sptr peak,
                                      double guessedfwhm);

    /// Fit background-removed peak by Gaussian
    bool doFitGaussianPeak(DataObjects::Workspace2D_sptr dataws, size_t m_wsIndex, double in_center,
                           double leftfwhm, double rightfwhm, double& center, double& sigma, double& height);

    /// Create a Workspace2D for fitted peaks (pattern)
    DataObjects::Workspace2D_sptr genOutputFittedPatternWorkspace(std::vector<double> pattern, int m_wsIndex);

    /// Calcualte the value of a single peak in a given range.
    void calculate1PeakGroup(vector<size_t> peakindexes, BackgroundFunction_sptr background);

    /// Parse the fitting result
    std::string parseFitResult(API::IAlgorithm_sptr fitalg, double& chi2);

    /// Calculate a Bragg peak's centre in TOF from its Miller indices
    double calculatePeakCentreTOF(int h, int k, int l);

    /// Get parameter value from m_instrumentParameters
    double getParameter(std::string parname);

    /// Fit peaks in the same group (i.e., single peak or overlapped peaks)
    void fitPeaksGroup(vector<size_t> peakindexes);

    /// Data
    API::MatrixWorkspace_sptr m_dataWS;

    // Map for all peaks to fit individually
    // Disabled std::map<std::vector<int>, CurveFitting::BackToBackExponential_sptr> m_peaksmap;

    /// Sorted vector for peaks.  double = d_h, vector = (HKL), peak
    vector<pair<double, pair<vector<int>, BackToBackExponential_sptr> > > m_peaks;

    /// Map for function (instrument parameter)
    std::map<std::string, double> m_instrumentParmaeters;

    /// Data for each individual peaks. (HKL)^2, vector index, function values
    std::vector<double> m_peakData;

    /// Peak parmeter names
    std::vector<std::string> mPeakParameterNames;

    /// TOF vector of data workspace to process with
    int m_wsIndex;

    /// Flag to use given Bragg peaks' centre in TOF
    bool m_useGivenTOFh;

    /// Flag to show whether input instrument parameters is trustful
    bool m_confidentInInstrumentParameters;

    /// Minimum HKL
    vector<int> m_minimumHKL;

    /// Number of peaks to fit lower to minimum HKL
    int m_numPeaksLowerToMin;

    std::vector<size_t> m_indexGoodFitPeaks;
    std::vector<double> m_chi2GoodFitPeaks;

    /// Fit mode
    enum {ROBUSTFIT, TRUSTINPUTFIT} m_fitMode;

    /// Choice to generate peak profile paramter starting value
    enum {HKLCALCULATION, FROMBRAGGTABLE} m_genPeakStartingValue;

    /// Right most peak HKL
    vector<int> m_rightmostPeakHKL;

    /// Right most peak's left boundary
    double m_rightmostPeakLeftBound;

    /// Right most peak's right boundary
    double m_rightmostPeakRightBound;

    /// Centres of peaks from input
    vector<double> m_inputPeakCentres;

  };

  /** Formular for linear iterpolation: X = [(xf-x0)*Y - (xf*y0-x0*yf)]/(yf-y0)
    */
  inline double linearInterpolateX(double x0, double xf, double y0, double yf, double y)
  {
    double x = ((xf-x0)*y - (xf*y0-x0*yf))/(yf-y0);
    return x;
  }

  /** Formula for linear interpolation: Y = ( (xf*y0-x0*yf) + x*(yf-y0) )/(xf-x0)
    */
  inline double linearInterpolateY(double x0, double xf, double y0, double yf, double x)
  {
    double y = ((xf*y0-x0*yf) + x*(yf-y0))/(xf-x0);
    return y;
  }

  Workspace2D_sptr buildPartialWorkspace(MatrixWorkspace_sptr sourcews, size_t workspaceindex,
                                         double leftbound, double rightbound);

  /// Estimate background for a pattern in a coarse mode
  void estimateBackgroundCoarse(Workspace2D_sptr dataws, BackgroundFunction_sptr background,
                                size_t wsindexraw, size_t wsindexbkgd, size_t wsindexpeak);

  /// Estimate peak parameters;
  bool estimatePeakParameters(Workspace2D_sptr dataws, size_t wsindex, double& centre, double& height, double& fwhm,
                              string& errmsg);

  /// Find maximum value
  size_t findMaxValue(const MantidVec Y);

  /// Find maximum value
  size_t findMaxValue(MatrixWorkspace_sptr dataws, size_t wsindex, double leftbound, double rightbound);

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_FITPOWDERDIFFPEAKS2_H_ */
