// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_FITPEAK_H_
#define MANTID_ALGORITHMS_FITPEAK_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace HistogramData {
class HistogramX;
class HistogramY;
} // namespace HistogramData

namespace Algorithms {
/// Get an index of a value in a sorted vector.  The index should be the item
/// with value nearest to X
size_t getIndex(const HistogramData::HistogramX &vecx, double x);

/** FitOneSinglePeak: a class to perform peak fitting on a single peak
 */
class DLLExport FitOneSinglePeak : public API::Algorithm {
public:
  /// Constructor
  FitOneSinglePeak();

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fit a single peak with checking mechanism. ";
  }

  /// Set workspaces
  void setWorskpace(API::MatrixWorkspace_sptr dataws, size_t wsindex);

  /// Set fitting method
  void setFittingMethod(std::string minimizer, std::string costfunction);

  /// Set functions
  void setFunctions(API::IPeakFunction_sptr peakfunc,
                    API::IBackgroundFunction_sptr bkgdfunc);

  /// Set fit range
  void setFitWindow(double leftwindow, double rightwindow);

  /// Set peak range
  void setPeakRange(double xpeakleft, double xpeakright);

  /// Set peak width to guess
  void setupGuessedFWHM(double usrwidth, int minfwhm, int maxfwhm, int stepsize,
                        bool fitwithsteppedfwhm);

  void setFitPeakCriteria(bool usepeakpostol, double peakpostol);

  std::string getDebugMessage();

  /// Fit peak and background together
  bool simpleFit();

  /// Fit peak first considering high background
  void highBkgdFit();

  /// Get fitting error for peak function
  std::map<std::string, double> getPeakError();

  /// Get fitting error for background function
  std::map<std::string, double> getBackgroundError();

  /// Get cost function value from fitting
  double getFitCostFunctionValue();

  /// Generate a partial workspace at fit window
  API::MatrixWorkspace_sptr genFitWindowWS();

private:
  /// Name
  const std::string name() const override { return "FitOneSinglePeak"; }

  /// Version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Fit"}; }
  /// Init
  void init() override;
  /// Exec
  void exec() override;

  /// Check whether it is ready to fit
  bool hasSetupToFitPeak(std::string &errmsg);

  /// Estimate the peak height from a set of data containing pure peaks
  double estimatePeakHeight(API::IPeakFunction_const_sptr peakfunc,
                            API::MatrixWorkspace_sptr dataws, size_t wsindex,
                            size_t ixmin, size_t ixmax);

  /// Check a peak function whether it is valid comparing to user specified
  /// criteria
  double checkFittedPeak(API::IPeakFunction_sptr peakfunc, double costfuncvalue,
                         std::string &errorreason);

  /// Fit peak function (flexible)
  double fitPeakFunction(API::IPeakFunction_sptr peakfunc,
                         API::MatrixWorkspace_sptr dataws, size_t wsindex,
                         double startx, double endx);

  /// Fit function in single domain
  double fitFunctionSD(API::IFunction_sptr fitfunc,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax);

  /// Calculate chi-square of a single domain function
  double calChiSquareSD(API::IFunction_sptr fitfunc,
                        API::MatrixWorkspace_sptr dataws, size_t wsindex,
                        double xmin, double xmax);

  /// Fit peak and background composite function
  double fitCompositeFunction(API::IPeakFunction_sptr peakfunc,
                              API::IBackgroundFunction_sptr bkgdfunc,
                              API::MatrixWorkspace_sptr dataws, size_t wsindex,
                              double startx, double endx);

  /// Fit function in multiple-domain
  double fitFunctionMD(API::IFunction_sptr fitfunc,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       std::vector<double> vec_xmin,
                       std::vector<double> vec_xmax);
  /// remove background
  void removeBackground(API::MatrixWorkspace_sptr purePeakWS);

  /// Process and store fit result
  void processNStoreFitResult(double rwp, bool storebkgd);

  /// Back up fit result
  std::map<std::string, double> backup(API::IFunction_const_sptr func);

  void pop(const std::map<std::string, double> &funcparammap,
           API::IFunction_sptr func);

  /// Store function fitting error
  std::map<std::string, double>
  storeFunctionError(const API::IFunction_const_sptr &func);

  API::IBackgroundFunction_sptr
  fitBackground(API::IBackgroundFunction_sptr bkgdfunc);

  /// Flag to show whether fitting parameters are set
  bool m_fitMethodSet;
  /// Flag whether the peak range is set
  bool m_peakRangeSet;
  /// Flag whether the peak width is set
  bool m_peakWidthSet;
  /// Peak widnow is set up
  bool m_peakWindowSet;
  /// Flag to apply peak position tolerance
  bool m_usePeakPositionTolerance;

  /// Peak function
  API::IPeakFunction_sptr m_peakFunc;
  /// Background function
  API::IBackgroundFunction_sptr m_bkgdFunc;

  /// Input data workspace
  API::MatrixWorkspace_sptr m_dataWS;
  /// Input worskpace index
  size_t m_wsIndex;

  /// Lower boundary of fitting range
  double m_minFitX;
  /// Upper boundary of fitting range
  double m_maxFitX;
  /// index of m_minFitX
  size_t i_minFitX;
  /// index of m_maxFitX
  size_t i_maxFitX;

  /// peak left boundary (client-defined)
  double m_minPeakX;
  /// peak right boundary (client-defined)
  double m_maxPeakX;
  /// index of m_minPeakX
  size_t i_minPeakX;
  /// index of m_maxPeakX
  size_t i_maxPeakX;

  /// Best peak parameters
  std::map<std::string, double> m_bestPeakFunc;
  /// Best background parameters
  std::map<std::string, double> m_bestBkgdFunc;

  /// Backed up peak function parameters
  std::map<std::string, double> m_bkupPeakFunc;
  /// Backed up background function parameters
  std::map<std::string, double> m_bkupBkgdFunc;

  /// Fit error of peak function
  std::map<std::string, double> m_fitErrorPeakFunc;
  /// Fit error of background function
  std::map<std::string, double> m_fitErrorBkgdFunc;

  /// Minimzer
  std::string m_minimizer;
  /// Cost function
  std::string m_costFunction;

  std::vector<double> m_vecFWHM;

  /// Peak position tolerance
  double m_peakPositionTolerance;

  /// Peak centre provided by user
  double m_userPeakCentre;

  ///
  double m_bestRwp;

  /// Final goodness value (Rwp/Chi-square)
  double m_finalGoodnessValue;

  ///
  size_t m_numFitCalls;

  /// String stream
  std::stringstream m_sstream;
};

/** FitPeak : Fit a single peak
 */
class DLLExport FitPeak : public API::Algorithm {
public:
  FitPeak();

  /// Algorithm's name
  const std::string name() const override { return "FitPeak"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fit a single peak with checking mechanism. ";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Optimization"; }

private:
  void init() override;
  void exec() override;

  /// Process input propeties
  void processProperties();

  /// Check the input properties and functions
  void prescreenInputData();

  /// Set up the output workspaces
  void setupOutput(const std::map<std::string, double> &m_fitErrorPeakFunc,
                   const std::map<std::string, double> &m_fitErrorBkgdFunc);

  /// Fit a single peak function with pure peak workspace
  double fitPeakFunction(API::IPeakFunction_sptr peakfunc,
                         API::MatrixWorkspace_sptr dataws, size_t wsindex,
                         double startx, double endx);

  /// Fit background with multiple domain
  API::IBackgroundFunction_sptr
  fitBackground(API::IBackgroundFunction_sptr bkgdfunc);

  /// Fit peak and background composite function
  double fitCompositeFunction(API::IPeakFunction_sptr peakfunc,
                              API::IBackgroundFunction_sptr bkgdfunc,
                              API::MatrixWorkspace_sptr dataws, size_t wsindex,
                              double startx, double endx);

  /// Make a pure peak WS in the fit window region
  void makePurePeakWS(API::MatrixWorkspace_sptr purePeakWS);

  /// Estimate the peak height from a set of data containing pure peaks
  double estimatePeakHeight(API::IPeakFunction_sptr peakfunc,
                            API::MatrixWorkspace_sptr dataws, size_t wsindex,
                            size_t ixmin, size_t ixmax);

  /// Fit a function.
  double fitFunctionSD(API::IFunction_sptr fitfunc,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax, bool calmode);

  /// Fit a function in multi-domain
  double fitFunctionMD(API::IFunction_sptr fitfunc,
                       API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       std::vector<double> vec_xmin,
                       std::vector<double> vec_xmax);

  /// Process and store fit result
  void processNStoreFitResult(double rwp, bool storebkgd);

  /// Set up a vector of guessed FWHM
  void setupGuessedFWHM(std::vector<double> &vec_FWHM);

  /// Pop
  void pop(const std::map<std::string, double> &funcparammap,
           API::IFunction_sptr func);

  /// Backup data
  API::MatrixWorkspace_sptr genPurePeakWS();

  /// Create functions
  void createFunctions();

  /// Check the fitted peak value to see whether it is valud
  double checkFittedPeak(API::IPeakFunction_sptr peakfunc, double costfuncvalue,
                         std::string &errorreason);

  /// Generate table workspace
  DataObjects::TableWorkspace_sptr
  genOutputTableWS(API::IPeakFunction_sptr peakfunc,
                   std::map<std::string, double> peakerrormap,
                   API::IBackgroundFunction_sptr bkgdfunc,
                   std::map<std::string, double> bkgderrormap);

  /// Add function's parameter names after peak function name
  std::vector<std::string>
  addFunctionParameterNames(std::vector<std::string> funcnames);

  /// Parse peak type from full peak type/parameter names string
  std::string parseFunctionTypeFull(const std::string &fullstring,
                                    bool &defaultparorder);

  /// Input data workspace
  API::MatrixWorkspace_sptr m_dataWS;
  size_t m_wsIndex;

  /// Peak function
  API::IPeakFunction_sptr m_peakFunc;
  /// Background function
  API::IBackgroundFunction_sptr m_bkgdFunc;
  /// Minimum fit position
  double m_minFitX;
  /// Maximum fit position
  double m_maxFitX;
  /// Minimum peak position
  double m_minPeakX;
  /// Maximum peak position
  double m_maxPeakX;

  /// Vector index of m_minFitX
  size_t i_minFitX;
  /// Vector index of m_maxFitX
  size_t i_maxFitX;
  /// Vector index of m_minPeakX
  size_t i_minPeakX;
  /// Vector index of m_maxPeakX
  size_t i_maxPeakX;

  /// fitting strategy
  bool m_fitBkgdFirst;

  /// output option
  bool m_outputRawParams;

  /// User guessed FWHM
  double m_userGuessedFWHM;
  /// User guessed peak centre
  double m_userPeakCentre;

  /// Minimum guessed peak width (pixels)
  int m_minGuessedPeakWidth;
  /// Maximum guessed peak width (pixels)
  int m_maxGuessedPeakWidth;
  /// Step width of tried FWHM
  int m_fwhmFitStep;
  /// Flag about guessed FWHM (pixels)
  bool m_fitWithStepPeakWidth;

  /// Use peak position tolerance as a criterial for peak fit
  bool m_usePeakPositionTolerance;
  /// Tolerance on peak positions as criteria
  double m_peakPositionTolerance;

  DataObjects::TableWorkspace_sptr m_peakParameterTableWS;
  DataObjects::TableWorkspace_sptr m_bkgdParameterTableWS;

  /// Peak
  std::vector<std::string> m_peakParameterNames;
  /// Background
  std::vector<std::string> m_bkgdParameterNames;

  /// Minimizer
  std::string m_minimizer;

  /// Storage map for background function
  std::map<std::string, double> m_bkupBkgdFunc;
  /// Storage map for peak function
  std::map<std::string, double> m_bkupPeakFunc;
  /// Best fitted peak function
  std::map<std::string, double> m_bestPeakFunc;
  /// Best background function
  std::map<std::string, double> m_bestBkgdFunc;
  /// Best Rwp ...
  double m_bestRwp;

  /// Final
  double m_finalGoodnessValue;

  /// Backups
  std::vector<double> m_vecybkup;
  std::vector<double> m_vecebkup;

  std::string m_costFunction;

  /// Fitting result
  // std::map<std::string, double> m_fitErrorPeakFunc;
  // std::map<std::string, double> m_fitErrorBkgdFunc;

  /// Option on output
  bool m_lightWeightOutput;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FITPEAK_H_ */
