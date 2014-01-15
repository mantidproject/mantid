#ifndef MANTID_ALGORITHMS_FITPEAK_H_
#define MANTID_ALGORITHMS_FITPEAK_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidDataObjects/TableWorkspace.h"


namespace Mantid
{
namespace Algorithms
{
  /// Get an index of a value in a sorted vector.  The index should be the item with value nearest to X
  size_t getVectorIndex(const MantidVec& vecx, double x);

  /** FitOneSinglePeak: a class to perform peak fitting on a single peak
    */
  class DLLExport FitOneSinglePeak : public API::Algorithm
  {
  public:
    /// Constructor
    FitOneSinglePeak();

    /// Desctructor
    virtual ~FitOneSinglePeak();

    /// Set fitting method
    void setFittingMethod(std::string minimizer, std::string costfunction);

    /// Set functions
    void setFunctions(API::IPeakFunction_sptr peakfunc, API::IBackgroundFunction_sptr bkgdfunc);

    /// Create functions from sctrach
    void createFunctions(std::string peaktype, std::string bkgdtype);

    /// Set workspaces
    void setWorskpace(API::MatrixWorkspace_const_sptr dataws, size_t wsindex);

    /// Set fit window
    // void setFitWindow(double xmin, double xmax);

    /// Set fit range
    void setFitWindow(double leftwindow, double rightwindow, double leftpeak,
                      double rightpeak);

    /// Set peak range
    void setPeakRange(double xpeakleft, double xpeakright);

    /// Set peak width to guess
    void setupGuessedFWHM(int width);

    /// Fit peak and background together
    bool simpleFit(size_t numsteps);

    /// Fit peak first considering high background
    bool highBkgdFit(size_t numsteps);

    /// Get peak
    API::IPeakFunction_sptr getPeakFunction();

    /// Get background
    API::IBackgroundFunction_sptr getBackgroundFunction();

    /// Estimate the peak height from a set of data containing pure peaks
    double estimatePeakHeight(API::IPeakFunction_sptr peakfunc, API::MatrixWorkspace_sptr dataws,
                              size_t wsindex, size_t ixmin, size_t ixmax);

    /// Fit peak function (flexible)
    double fitPeakFunction(API::IPeakFunction_sptr peakfunc, API::MatrixWorkspace_sptr dataws,
                        size_t wsindex, double startx, double endx);

    /// Generate a partial workspace at fit window
    API::MatrixWorkspace_sptr genFitWindowWS();

    /// remove background
    void removeBackground(API::MatrixWorkspace_sptr purePeakWS);

    void setPeakParameterValues();

    void setBackgroundParameterValues();


  private:

    ///
    virtual const std::string name() const;

    ///
    virtual int version() const;

    ///
    void init();

    ///
    void exec();

    void setupGuessedFWHM(std::vector<double>& vec_FWHM);

    ///
    double fitFunctionSD(API::IFunction_sptr fitfunc, API::MatrixWorkspace_const_sptr dataws, size_t wsindex,
                         double xmin, double xmax, bool calmode);

    ///
    void processNStoreFitResult(double, bool);

    void push(API::IFunction_const_sptr func, std::map<std::string, double>& funcparammap,
              std::map<std::string, double>& paramerrormap);

    void pop(const std::map<std::string, double>& funcparammap, API::IFunction_sptr func);

    API::IBackgroundFunction_sptr fitBackground(API::IBackgroundFunction_sptr bkgdfunc);


    /// Peak function
    API::IPeakFunction_sptr m_peakFunc;
    /// Background function
    API::IBackgroundFunction_sptr m_bkgdFunc;

    /// Input data workspace
    API::MatrixWorkspace_const_sptr m_dataWS;
    /// Input worskpace index
    size_t m_wsIndex;

    /// Lower boundary of fitting range
    double m_minFitX;
    /// Upper boundary of fitting range
    double m_maxFitX;

    ///
    double m_userGuessedFWHM;
    ///
    double m_minGuessedPeakWidth;
    ///
    double m_maxGuessedPeakWidth;
    ///
    double m_fwhmFitStep;

    /// Best peak parameters
    std::map<std::string, double> m_bestPeakFunc;
    /// Best background parameters
    std::map<std::string, double> m_bestBkgdFunc;

    /// Backed up peak function parameters
    std::map<std::string, double> m_bkupPeakFunc;
    /// Backed up background function parameters
    std::map<std::string, double> m_bkupBkgdFunc;

    ///
    std::string m_minimizer;
    ///
    std::string m_costFunction;

    ///
    size_t i_minFitX;
    ///
    size_t i_maxFitX;

    /// Log
    // Kernel::Logger& g_log;

  };



  /** FitPeak : Fit a single peak
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport FitPeak : public API::Algorithm
  {
  public:
    FitPeak();
    virtual ~FitPeak();

    /// Algorithm's name
    virtual const std::string name() const { return "FitPeak"; }
    /// Algorithm's version
    virtual int version() const { return (1); }
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Optimization"; }

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    void init();
    void exec();

    /// Process input propeties
    void processProperties();

    /// Check the input properties and functions
    void prescreenInputData();

    /// Fit peak in a simple one-step approach
    void fitPeakOneStep();

    /// Fit peak in a robust manner.  Multiple fit will be
    void fitPeakMultipleStep();

    /// Set up the output workspaces
    void setupOutput();

    /// Fit a single peak function with pure peak workspace
    double fitPeakFunction(API::IPeakFunction_sptr peakfunc, API::MatrixWorkspace_sptr dataws,
                          size_t wsindex, double startx, double endx);

    /// Fit background with multiple domain
    API::IBackgroundFunction_sptr fitBackground(API::IBackgroundFunction_sptr bkgdfunc);

    /// Fit peak and background composite function
    double fitCompositeFunction(API::IPeakFunction_sptr peakfunc, API::IBackgroundFunction_sptr bkgdfunc,
                              API::MatrixWorkspace_sptr dataws, size_t wsindex,
                              double startx, double endx);

    /// Make a pure peak WS in the fit window region
    void makePurePeakWS(API::MatrixWorkspace_sptr purePeakWS);

    /// Estimate the peak height from a set of data containing pure peaks
    double estimatePeakHeight(API::IPeakFunction_sptr peakfunc, API::MatrixWorkspace_sptr dataws,
                              size_t wsindex, size_t ixmin, size_t ixmax);

    /// Fit a function.
    double fitFunctionSD(API::IFunction_sptr fitfunc, API::MatrixWorkspace_sptr dataws,
                       size_t wsindex, double xmin, double xmax, bool calmode);

    /// Fit a function in multi-domain
    double fitFunctionMD(API::IFunction_sptr fitfunc, API::MatrixWorkspace_sptr dataws,
                         size_t wsindex, std::vector<double> vec_xmin, std::vector<double> vec_xmax);

    /// Process and store fit result
    void processNStoreFitResult(double rwp,  bool storebkgd);

    /// Set up a vector of guessed FWHM
    void setupGuessedFWHM(std::vector<double>& vec_FWHM);

    /// Push/store a fit result
    void push(API::IFunction_const_sptr func, std::map<std::string, double>& funcparammap,
              std::map<std::string, double>& paramerrormap);

    /// Pop
    void pop(const std::map<std::string, double>& funcparammap, API::IFunction_sptr func);

    /// Backup data
    API::MatrixWorkspace_sptr genPurePeakWS();

    /// Create functions
    void createFunctions();

#if 0
    /// Get an index of a value in a sorted vector.  The index should be the item with value nearest to X
    size_t getVectorIndex(const MantidVec& vecx, double x);
#endif

    /// Check the fitted peak value to see whether it is valud
    double checkFittedPeak(API::IPeakFunction_sptr peakfunc, double costfuncvalue, std::string& errorreason);

    /// Generate table workspace
    DataObjects::TableWorkspace_sptr genOutputTableWS(API::IPeakFunction_sptr peakfunc,
                                                      std::map<std::string, double> peakerrormap,
                                                      API::IBackgroundFunction_sptr bkgdfunc,
                                                      std::map<std::string, double> bkgderrormap);


    /// Add function's parameter names after peak function name
    std::vector<std::string> addFunctionParameterNames(std::vector<std::string> funcnames);

    /// Parse peak type from full peak type/parameter names string
    std::string parseFunctionTypeFull(const std::string& fullstring, bool &defaultparorder);

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
    std::map<std::string, double> m_fitErrorPeakFunc;
    std::map<std::string, double> m_fitErrorBkgdFunc;

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_FITPEAK_H_ */
