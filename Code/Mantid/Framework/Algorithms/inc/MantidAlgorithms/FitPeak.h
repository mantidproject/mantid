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

    /// Fit background with multiple domain
    API::IBackgroundFunction_sptr fitBackground(API::IBackgroundFunction_sptr bkgdfunc);

    /// Make a pure peak WS in the fit window region
    void makePurePeakWS(const std::vector<double>& vec_bkgd);

    /// Fit a function.
    double fitFunction(API::IFunction_sptr fitfunc, API::MatrixWorkspace_const_sptr dataws,
                       size_t wsindex, double xmin, double xmax,
                       std::vector<double>& vec_caldata);

    /// Set up a vector of guessed FWHM
    void setupGuessedFWHM(std::vector<double>& vec_FWHM);

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

    /// fitting strategy
    bool m_fitBkgdFirst;

    /// output option
    bool m_outputRawParams;

    /// Flag about guessed FWHM
    bool m_fitWithStepPeakWidth;

    /// Use peak position tolerance as a criterial for peak fit
    bool m_usePeakPositionTolerance;


  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_FITPEAK_H_ */
