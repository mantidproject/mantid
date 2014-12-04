#ifndef MANTID_CURVEFITTING_FITPOWDERDIFFPEAKS_H_
#define MANTID_CURVEFITTING_FITPOWDERDIFFPEAKS_H_

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
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid
{
namespace CurveFitting
{

  /** FitPowderDiffPeaks : Fit peaks in powder diffraction pattern.

    Mode Confident:
    * In this mode, the starting values of parameters except height will be given in input
      table workspace;
    * Use case 1: user has some pre-knowledge of the peak shape parameters, i.e, the analytical
                  function to describe all peaks.
    * Use case 2: user has no pre-knowledge of the peak shape parameters, but have some
                  single peaks fitted;  The best starting value/estimation is from its right peak
                  with proper fit
    Solution: Let them compete!

    Its application is to serve as the first step for refining powder diffractomer instrument
    parameters. Its output will be used by RefinePowderInstrumentParameters().
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport FitPowderDiffPeaks : public API::Algorithm
  {
  public:
    FitPowderDiffPeaks();
    virtual ~FitPowderDiffPeaks();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "FitPowderDiffPeaks";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Fit peaks in powder diffraction pattern. ";}


    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction";}

  private:
    
    /// Implement abstract Algorithm methods
    void init();
    /// Implement abstract Algorithm methods
    void exec();

    /// Process input properties
    void processInputProperties();

    /// Generate peaks from input table workspace
    void genPeaksFromTable(DataObjects::TableWorkspace_sptr peakparamws);

    /// Generate a peak
    BackToBackExponential_sptr genPeak(std::map<std::string, int> hklmap,
                                       std::map<std::string, double> parammap,
                                       std::map<std::string, std::string> bk2bk2braggmap,
                                       bool &good, std::vector<int> &hkl, double &d_h);

    /// Get (HKL) from a map; Return false if the information is incomplete
    bool getHKLFromMap(std::map<std::string, int> intmap, std::vector<int>& hkl);

    /// Import instrument parameters from (input) table workspace
    void importInstrumentParameterFromTable(DataObjects::TableWorkspace_sptr m_profileTable);

    /// Import Bragg peak table workspace
    void parseBraggPeakTable(DataObjects::TableWorkspace_sptr peakws,
                             std::vector<std::map<std::string, double> >& parammaps,
                             std::vector<std::map<std::string, int> >& hklmaps);

    /// Fit peaks
    void fitPeaksWithGoodStartingValues();

    /// Fit peaks in robust algorithm
    void fitPeaksRobust();

    /// Fit a single peak
    bool fitPeak(BackToBackExponential_sptr peak, BackgroundFunction_sptr background, double leftdev,
                 double rightdev, size_t m_wsIndex, double &chi2);

    //---------------------------------------------------------------------------

    /// Fit single peak in robust mode (no hint)
    bool fitSinglePeakRobust(BackToBackExponential_sptr peak, BackgroundFunction_sptr background,
                             double leftdev, double rightdev, std::map<std::string, double> rightpeakparammap,
                             double &finalchi2);

    /// Fit signle peak by Monte Carlo/simulated annealing
    bool fitSinglePeakSimulatedAnnealing(BackToBackExponential_sptr peak, std::vector<std::string> paramtodomc);

    /// Fit peak with confidence of the centre
    bool fitSinglePeakConfidentX(BackToBackExponential_sptr peak);

    /// Fit peak with trustful peak parameters
    bool fitSinglePeakConfident(BackToBackExponential_sptr peak, BackgroundFunction_sptr backgroundfunction,
                                double leftbound, double rightbound, double &chi2, bool &annhilatedpeak);

    /// Fit peak with confident parameters
    bool fitSinglePeakConfidentY(DataObjects::Workspace2D_sptr dataws, BackToBackExponential_sptr peak, double dampingfactor);

    /// Fit peaks with confidence in fwhm and etc.
    bool fitOverlappedPeaks(std::vector<BackToBackExponential_sptr> peaks,
                            BackgroundFunction_sptr backgroundfunction,
                            double gfwhm);

    /// Fit multiple (overlapped) peaks
    bool doFitMultiplePeaks(DataObjects::Workspace2D_sptr dataws, size_t wsindex,
                            API::CompositeFunction_sptr peaksfunc,
                            std::vector<BackToBackExponential_sptr> peakfuncs,
                            std::vector<bool>& vecfitgood, std::vector<double>& vecchi2s);

    /// Use Le Bail method to estimate and set the peak heights
    void estimatePeakHeightsLeBail(DataObjects::Workspace2D_sptr dataws, size_t wsindex,
                                   std::vector<BackToBackExponential_sptr> peaks);

    /// Set constraints on a group of overlapped peaks for fitting
    void setOverlappedPeaksConstraints(std::vector<BackToBackExponential_sptr> peaks);


    /// Fit 1 peak by 1 minimizer of 1 call of minimzer (simple version)
    bool doFit1PeakSimple(DataObjects::Workspace2D_sptr dataws, size_t workspaceindex,
                          BackToBackExponential_sptr peakfunction,
                          std::string minimzername,
                          size_t maxiteration, double &chi2);

    /// Fit 1 peak and background
    // bool doFit1PeakBackgroundSimple(DataObjects::Workspace2D_sptr dataws, size_t workspaceindex,
    // BackToBackExponential_sptr peakfunction,
    // BackgroundFunction_sptr backgroundfunction,
    // string minimzername, size_t maxiteration, double &chi2);

    /// Fit single peak with background to raw data
    bool doFit1PeakBackground(DataObjects::Workspace2D_sptr dataws, size_t wsindex,
                             BackToBackExponential_sptr peak,
                             BackgroundFunction_sptr backgroundfunction,
                             double &chi2);

    /// Fit 1 peak by using a sequential of minimizer
    bool doFit1PeakSequential(DataObjects::Workspace2D_sptr dataws, size_t workspaceindex,
                              BackToBackExponential_sptr peakfunction,
                              std::vector<std::string> minimzernames, std::vector<size_t> maxiterations,
                              std::vector<double> dampfactors, double& chi2);

    /// Fit N overlapped peaks in a simple manner
    bool doFitNPeaksSimple(DataObjects::Workspace2D_sptr dataws, size_t wsindex,
                           API::CompositeFunction_sptr  peaksfunc, std::vector<BackToBackExponential_sptr> peakfuncs,
                           std::string minimizername, size_t maxiteration, double &chi2);

    /// Store the function's parameter values to a map
    void storeFunctionParameters(API::IFunction_sptr function,
                                 std::map<std::string, double>& parammaps);

    /// Restore the function's parameter values from a map
    void restoreFunctionParameters(API::IFunction_sptr function, std::map<std::string, double> parammap);

    /// Calculate the range to fit peak/peaks group
    void calculatePeakFitBoundary(size_t ileftpeak, size_t irightpeak,
                                  double &peakleftboundary, double &peakrightboundary);

    //---------------------------------------------------------------------------

    /// Find max height (peak center)
    bool findMaxHeight(API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax, double& center, double& centerleftbound, double& centerrightbound, int &errordirection);

    /// Create data workspace for X0, A, B and S of peak with good fit
    DataObjects::Workspace2D_sptr genPeakParameterDataWorkspace();

    /// Generate output peak parameters workspace
    std::pair<DataObjects::TableWorkspace_sptr, DataObjects::TableWorkspace_sptr> genPeakParametersWorkspace();

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

    /// Observe peak range with hint from right peak's properties
    void observePeakRange(BackToBackExponential_sptr thispeak,
                          BackToBackExponential_sptr rightpeak, double refpeakshift,
                          double& peakleftbound, double& peakrightbound);

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
    std::pair<bool, double> doFitPeak_Old(DataObjects::Workspace2D_sptr dataws, BackToBackExponential_sptr peak,
                                      double guessedfwhm, bool calchi2);

    std::pair<bool, double> doFitPeak(DataObjects::Workspace2D_sptr dataws,BackToBackExponential_sptr peakfunction,
                                      double guessedfwhm);

    /// Fit background-removed peak by Gaussian
    bool doFitGaussianPeak(DataObjects::Workspace2D_sptr dataws, size_t m_wsIndex, double in_center,
                           double leftfwhm, double rightfwhm, double& center, double& sigma, double& height);

    /// Create a Workspace2D for fitted peaks (pattern)
    DataObjects::Workspace2D_sptr genOutputFittedPatternWorkspace(std::vector<double> pattern, int m_wsIndex);

    /// Calcualte the value of a single peak in a given range.
    void calculate1PeakGroup(std::vector<size_t> peakindexes, BackgroundFunction_sptr background);

    /// Parse the fitting result
    std::string parseFitResult(API::IAlgorithm_sptr fitalg, double& chi2, bool &fitsuccess);

    /// Calculate a Bragg peak's centre in TOF from its Miller indices
    double calculatePeakCentreTOF(int h, int k, int l);

    /// Get parameter value from m_instrumentParameters
    double getParameter(std::string parname);

    /// Fit peaks in the same group (i.e., single peak or overlapped peaks)
    void fitPeaksGroup(std::vector<size_t> peakindexes);

    /// Build partial workspace for fitting
    DataObjects::Workspace2D_sptr buildPartialWorkspace(API::MatrixWorkspace_sptr sourcews,
                                                        size_t workspaceindex,
                                                        double leftbound, double rightbound);

    /// Plot a single peak to output vector
    void plotFunction(API::IFunction_sptr peakfunction, BackgroundFunction_sptr background,
                      API::FunctionDomain1DVector domain);

    //-----------------------------------------------------------------------------------------------

    /// Data
    API::MatrixWorkspace_sptr m_dataWS;

    /// Bragg peak parameter
    DataObjects::TableWorkspace_sptr m_peakParamTable;

    /// Instrument profile parameter table workspace
    DataObjects::TableWorkspace_sptr m_profileTable;

    // Map for all peaks to fit individually
    // Disabled std::map<std::vector<int>, CurveFitting::BackToBackExponential_sptr> m_peaksmap;

    /// Sorted vector for peaks.  double = d_h, vector = (HKL), peak
    std::vector<std::pair<double, std::pair<std::vector<int>, BackToBackExponential_sptr> > > m_vecPeakFunctions;

    /// Peak fitting information
    std::vector<double> m_peakFitChi2;

    /// Peak fitting status
    std::vector<bool> m_goodFit;

    /// Map for function (instrument parameter)
    std::map<std::string, double> m_instrumentParmaeters;

    /// Data for each individual peaks. (HKL)^2, vector index, function values
    std::vector<double> m_peakData;

    /// Peak parmeter names
    std::vector<std::string> mPeakParameterNames;

    /// TOF vector of data workspace to process with
    int m_wsIndex;

    /// TOF Min and TOF Max
    double m_tofMin;
    double m_tofMax;

    /// Flag to use given Bragg peaks' centre in TOF
    bool m_useGivenTOFh;

    /// Flag to show whether input instrument parameters is trustful
    bool m_confidentInInstrumentParameters;

    /// Minimum HKL
    std::vector<int> m_minimumHKL;

    /// Number of peaks to fit lower to minimum HKL
    int m_numPeaksLowerToMin;

    std::vector<size_t> m_indexGoodFitPeaks;
    std::vector<double> m_chi2GoodFitPeaks;

    /// Fit mode
    enum {ROBUSTFIT, TRUSTINPUTFIT} m_fitMode;

    /// Choice to generate peak profile paramter starting value
    enum {HKLCALCULATION, FROMBRAGGTABLE} m_genPeakStartingValue;

    /// Right most peak HKL
    std::vector<int> m_rightmostPeakHKL;

    /// Right most peak's left boundary
    double m_rightmostPeakLeftBound;

    /// Right most peak's right boundary
    double m_rightmostPeakRightBound;

    /// Minimum peak height for peak to be refined
    double m_minPeakHeight;

    /// Unit cell of powder crystal
    Geometry::UnitCell m_unitCell;

    /// Fit peak + background as the last step
    bool m_fitPeakBackgroundComposite;

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

  /// Estimate background for a pattern in a coarse mode
  void estimateBackgroundCoarse(DataObjects::Workspace2D_sptr dataws, BackgroundFunction_sptr background,
                                size_t wsindexraw, size_t wsindexbkgd, size_t wsindexpeak);

  /// Estimate peak parameters;
  bool observePeakParameters(DataObjects::Workspace2D_sptr dataws, size_t wsindex, double& centre, double& height, double& fwhm,
                             std::string& errmsg);

  /// Find maximum value
  size_t findMaxValue(const Mantid::MantidVec Y);

  /// Find maximum value
  size_t findMaxValue(API::MatrixWorkspace_sptr dataws, size_t wsindex, double leftbound, double rightbound);

  /// Get function parameter name, value and etc information in string
  std::string getFunctionInfo(API::IFunction_sptr function);

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_FITPOWDERDIFFPEAKS_H_ */
