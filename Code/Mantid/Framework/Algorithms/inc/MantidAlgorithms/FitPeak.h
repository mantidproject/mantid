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

    /// Fit a single peak function with pure peak workspace
    double fitPeakFuncion(API::IPeakFunction_sptr peakfunc, API::MatrixWorkspace_const_sptr dataws,
                          size_t wsindex, double startx, double endx);

    /// Fit background with multiple domain
    API::IBackgroundFunction_sptr fitBackground(API::IBackgroundFunction_sptr bkgdfunc);

    /// Fit peak and background composite function
    bool fitCompositeFunction(API::IPeakFunction_sptr peakfunc, API::IBackgroundFunction_sptr bkgdfunc,
                              API::MatrixWorkspace_const_sptr dataws, size_t wsindex,
                              double startx, double endx);

    /// Make a pure peak WS in the fit window region
    void makePurePeakWS(const std::vector<double>& vec_bkgd);

    /// Fit a function.
    double fitFunction(API::IFunction_sptr fitfunc, API::MatrixWorkspace_const_sptr dataws,
                       size_t wsindex, double xmin, double xmax,
                       std::vector<double>& vec_caldata);

    /// Fit a function in multi-domain
    double fitFunctionMD(API::IFunction_sptr fitfunc, API::MatrixWorkspace_const_sptr dataws,
                         size_t wsindex, std::vector<double> xmin, std::vector<double> xmax,
                         std::vector<double>& vec_caldata);

    /// Process and store fit result
    void processNStoreFitResult(double rwp);

    /// Set up a vector of guessed FWHM
    void setupGuessedFWHM(std::vector<double>& vec_FWHM);

    /// Push/store a fit result
    void push(API::IFunction_const_sptr func, std::map<std::string, double>& funcparammap);

    /// Pop
    void pop(const std::map<std::string, double>& funcparammap, API::IFunction_sptr func);

    /// Backup data
    void backupOriginalData(std::vector<double>& vecy, std::vector<double> &vece);

    /// Create functions
    void createFunctions();

    /// Get an index of a value in a sorted vector.  The index should be the item with value nearest to X
    size_t getVectorIndex(const MantidVec& vecx, double x);

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
    double m_maxGuessedPeakWidth;
    /// Step width of tried FWHM
    int m_fwhmFitStep;
    /// Flag about guessed FWHM (pixels)
    bool m_fitWithStepPeakWidth;

    /// Use peak position tolerance as a criterial for peak fit
    bool m_usePeakPositionTolerance;
    /// Tolerance on peak positions as criteria
    double m_peakPositionTolerance;

    /// Function information
    std::string m_peakFuncType;
    std::string m_backgroundType;

    DataObjects::TableWorkspace_sptr m_parameterTableWS;

    /// Minimizer
    std::string m_minimizer;

    /// Storage map for background function
    std::map<std::string, double> m_bkupBkgdFunc;
    /// Storage map for peak function
    std::map<std::string, double> m_bkupPeakFunc;
    /// Best fitted peak function
    std::map<std::string, double> m_bestPeakFunc;
    /// Best Rwp ...
    double m_bestRwp;

    /// Backups
    std::vector<double> m_vecybkup;
    std::vector<double> m_vecebkup;


  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_FITPEAK_H_ */
