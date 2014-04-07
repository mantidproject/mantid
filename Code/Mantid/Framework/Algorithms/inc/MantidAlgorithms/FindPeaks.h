#ifndef MANTID_ALGORITHMS_FINDPEAKS_H_
#define MANTID_ALGORITHMS_FINDPEAKS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
// #include "MantidAPI/IFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/** This algorithm searches for peaks in a dataset.
    The method used is detailed in: M.A.Mariscotti, NIM 50 (1967) 309.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to search for peaks. </LI>
    <LI> PeaksList      - The name of the TableWorkspace in which to store the list of peaks found. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> fwhm - The number of points covered on average by the fwhm of a peak (default 7) </LI>
    <LI> Tolerance - Sets the strictness desired in meeting the conditions on peak candidates (default 4, Mariscotti recommended 2) </LI>
    <LI> WorkspaceIndex - The spectrum to search for peaks. Will search all spectra if absent. </LI>
    </UL>
    
    @author Russell Taylor, Tessella Support Services plc
    @date 25/11/2008

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

class DLLExport FindPeaks : public API::Algorithm
{
public:
  /// Constructor
  FindPeaks();
  /// Virtual destructor
  virtual ~FindPeaks() {if(m_progress) delete m_progress; m_progress=NULL;}
  /// Algorithm's name
  virtual const std::string name() const { return "FindPeaks"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Optimization\\PeakFinding"; }
  /// needed by FindPeaksBackground
  int getVectorIndex(const MantidVec &vecX, double x);

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  void init();
  void exec();

  /// Process algorithm's properties
  void processAlgorithmProperties();

  /// Find peaks by searching peak position using Mariscotti algorithm
  void findPeaksUsingMariscotti();

  /// Find peaks according to given peak positions
  void findPeaksGivenStartingPoints(const std::vector<double> &peakcentres, const std::vector<double> &fitwindows);

  /// Methods searving for findPeaksUsingMariscotti()
  API::MatrixWorkspace_sptr calculateSecondDifference(const API::MatrixWorkspace_const_sptr &input);
  void smoothData(API::MatrixWorkspace_sptr &WS, const int &w);
  void calculateStandardDeviation(const API::MatrixWorkspace_const_sptr &input, const API::MatrixWorkspace_sptr &smoothed, const int &w);
  long long computePhi(const int& w) const;

  /// Fit peak confined in a given window (x-min, x-max)
  void fitPeakInWindow(const API::MatrixWorkspace_sptr &input, const int spectrum, const double centre, const double xmin, const double xmax);

  /// Fit peak by given/guessed FWHM
  void fitPeakGivenFWHM(const API::MatrixWorkspace_sptr &input, const int spectrum, const double center_guess, const int fitWidth);

  /// Fit peak: this is a basic peak fit function as a root function for all different type of user input
  void fitSinglePeak(const API::MatrixWorkspace_sptr &input, const int spectrum, const int i_min, const int i_max, const int i_centre);

  void fitPeakHighBackground(const API::MatrixWorkspace_sptr &input, const size_t spectrum, int i_centre, int i_min, int i_max,
                             double &in_bg0, double &in_bg1, double &in_bg2, int i_peakmin, int i_peakmax);

  void fitPeakOneStep(const API::MatrixWorkspace_sptr &input, const int spectrum, const int& i0, const int& i2, const int& i4,
      const double& in_bg0, const double& in_bg1, const double& in_bg2);

  /// Add a new row in output TableWorkspace containing information of the fitted peak+background
  void addInfoRow(const size_t spectrum, const API::IPeakFunction_const_sptr& peakfunction,
                  const API::IBackgroundFunction_sptr& bkgdfunction,
                  const bool isoutputraw,
                  const double mincost);

  /// Add the fit record (failure) to output workspace
  void addNonFitRecord(const size_t spectrum);

  void updateFitResults(API::IAlgorithm_sptr fitAlg, std::vector<double> &bestEffparams, std::vector<double> &bestRawparams, double &mincost, const double expPeakPos, const double expPeakHeight);

  void createFunctions();
  // int getBackgroundOrder();
  /// Create a background function
  API::IFunction_sptr createBackgroundFunction(const double a0, const double a1, const double a2);

  /// Fit background functions
  bool fitBackground(const MantidVec& X, const MantidVec& Y, const MantidVec& E,
                     size_t ileft, size_t iright, size_t imin, size_t imax,
                     double &chi2);

  /// Fit a single peak with background fixed
  double fitPeakBackgroundFunction(API::IFunction_sptr peakbkgdfunc, API::MatrixWorkspace_sptr dataws, size_t wsindex, double startx, double endx, std::string constraint, double &init_rwp);

  /// Get function parameters from a function to a map
  std::map<std::string, double> getFunctionParameters(API::IFunction_sptr func);

  /// Set parameters to a peak function
  void setParameters(API::IFunction_sptr peak, double height, double centre, double sigma, double centre_lowerbound, double centre_upperbound);


  /// Set parameters values to a peak function
  void setFunctionParameterValue(API::IFunction_sptr function, std::map<std::string, double> parvalues);

  /// Set boundary/contraint on peak's centre
  std::string makePeakCentreConstraint(API::IFunction_sptr peak, double peakleftboundary, double peakrightboundary, bool composite);

  void estimateBackground(const MantidVec& X, const MantidVec& Y, const size_t i_min, const size_t i_max,
                                double& out_bg0, double& out_bg1, double& out_bg2);

  std::string estimatePeakParameters(const MantidVec& vecX, const MantidVec& vecY,
                              size_t i_min, size_t i_max, double& centre, double& height, double& fwhm);

  /// Calulate a function with given data range, and its goodness of fit, Rwp.
  double calculateFunctionRwp(API::IFunction_sptr function, API::MatrixWorkspace_sptr dataws,
                              size_t wsindex, double startx, double endx);

  /// Get best result from a set of fitting result
  int getBestResult(std::vector<double> vecRwp);

  void addFittedFunction(API::IFunction_sptr fitfunction, size_t ileft, size_t iright);

  /// Check the GSL fit status message to determine whether the fit is successful or not
  bool isFitSuccessful(std::string fitstatus);


  /// Generate a table workspace for output peak parameters
  void generateOutputPeakParameterTable();

  std::vector<double> getStartingPeakValues();
  std::vector<double> getStartingBkgdValues();

  void findPeakBackground(const API::MatrixWorkspace_sptr& input, int spectrum, size_t i_min, size_t i_max,
                          std::vector<double>& vecBkgdParamValues, std::vector<double>& vecpeakrange);

  double callFitPeak(const API::MatrixWorkspace_sptr& dataws, int wsindex,
                     const API::IPeakFunction_sptr peakfunction,
                     const API::IBackgroundFunction_sptr backgroundfunction,
                     const std::vector<double>& vec_fitwindow,
                     const std::vector<double>& vec_peakrange, int minGuessedFWHM, int maxGuessFWHM,
                     int guessedFWHMStep); //, std::vector<double>& vec_fittedpeakparvalues,
  // std::vector<double>& vec_fittedbkgdparvalues);

  std::vector<std::string> m_peakParameterNames;
  std::vector<std::string> m_bkgdParameterNames;
  size_t m_bkgdOrder;

  /// The number of smoothing iterations. Set to 5, the optimum value according to Mariscotti.
  static const int g_z = 5;
  
  /// Storage of the peak data
  API::ITableWorkspace_sptr m_outPeakTableWS;
  /// Progress reporting
  API::Progress* m_progress;

  //Properties saved in the algo.
  API::MatrixWorkspace_sptr m_dataWS; ///<workspace to check for peaks
  int m_inputPeakFWHM; ///<holder for the requested peak FWHM
  int index; ///<list of workspace indicies to check
  bool singleSpectrum; ///<flag for if only a single spectrum is present
  bool m_highBackground; ///<flag for find relatively weak peak in high background
  bool m_rawPeaksTable; ///<flag for whether the output is the raw peak parameters or effective (centre, width, height)
  std::size_t m_numTableParams; //<Number of parameters in the output table workspace
  bool m_searchPeakPos; ///<flag to search for peak in the window
  std::string m_peakFuncType; //< The name of the peak function to fit
  std::string m_backgroundType; //< The type of background to fit

  // Peaks positions
  std::vector<double> m_vecPeakCentre;
  std::vector<double> m_vecFitWindows;

  // Functions for reused 
  API::IBackgroundFunction_sptr m_backgroundFunction;
  API::IPeakFunction_sptr m_peakFunction;

  int m_minGuessedPeakWidth;
  int m_maxGuessedPeakWidth;
  int m_stepGuessedPeakWidth;

  bool m_usePeakPositionTolerance;
  double m_peakPositionTolerance;

  bool m_usePeakHeightTolerance;
  double m_peakHeightTolerance;

  std::vector<API::IFunction_sptr> m_fitFunctions;
  std::vector<size_t> m_peakLeftIndexes;
  std::vector<size_t> m_peakRightIndexes;

  std::string m_minimizer;
  std::string m_costFunction;

  /// Minimum peak height
  double m_minHeight;

};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_FINDPEAKS_H_*/
