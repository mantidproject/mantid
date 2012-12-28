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

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

  /** FitPowderDiffPeaks : Fit peaks in powder diffraction pattern.

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
  class DLLExport FitPowderDiffPeaks : public API::Algorithm
  {
  public:
    FitPowderDiffPeaks();
    virtual ~FitPowderDiffPeaks();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "FitPowderDiffPeaks";}

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
    void genPeaksFromTable(DataObjects::TableWorkspace_sptr peakparamws,
                                std::map<std::vector<int>, CurveFitting::BackToBackExponential_sptr>& peaks);

    /// Import instrument parameters from (input) table workspace
    void importParametersFromTable(DataObjects::TableWorkspace_sptr parameterWS, std::map<std::string, double>& parameters);

    /// Fit peaks
    void fitPeaks(int workspaceindex, std::vector<std::vector<int> >& goodfitpeaks, std::vector<double> &goodfitchi2);

    /// Fit a single peak
    bool fitPeak(BackToBackExponential_sptr peak, BackgroundFunction_sptr background, double leftdev,
                 double rightdev, size_t workspaceindex, double &chi2);

    /// Find max height (peak center)
    bool findMaxHeight(API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax, double& center, double& centerleftbound, double& centerrightbound, int &errordirection);

    /// Generate output peak parameters workspace
    std::pair<DataObjects::TableWorkspace_sptr, DataObjects::TableWorkspace_sptr> genPeakParametersWorkspace(
        std::vector<std::vector<int> > goodfitpeaks, std::vector<double> goodfitchi2s);

    /// Crop data workspace
    void cropWorkspace(double tofmin, double tofmax);

    /// Parse Fit() output parameter workspace
    std::string parseFitParameterWorkspace(API::ITableWorkspace_sptr paramws);

    /// Build a partial workspace from source
    DataObjects::Workspace2D_sptr buildPartialWorkspace(API::MatrixWorkspace_sptr sourcews, size_t workspaceindex,
                                                        double leftbound, double rightbound);

    /// Estimate background
    void estimateBackground(DataObjects::Workspace2D_sptr dataws);

    /// Subtract background (This is an operation within the specially defined data workspace for peak fitting)
    void subtractBackground(DataObjects::Workspace2D_sptr dataws);

    /// Estimate FWHM for the peak observed
    bool estimateFWHM(DataObjects::Workspace2D_sptr dataws, size_t wsindex, double tof_h, double& leftfwhm, double& rightfwhm);

    /// Fit background function by removing the peak properly
    bool doFitBackground(DataObjects::Workspace2D_sptr dataws, BackgroundFunction_sptr background,
                         double leftpeakbound, double rightpeakbound);

    /// Fit single peak without background
    std::pair<bool, double> doFitPeak(DataObjects::Workspace2D_sptr dataws, CurveFitting::BackToBackExponential_sptr peak, double tof_h,
                                      double leftfwhm, double rightfwhm);

    /// Fit background-removed peak by Gaussian
    bool doFitGaussianPeak(DataObjects::Workspace2D_sptr dataws, size_t workspaceindex, double in_center,
                           double leftfwhm, double rightfwhm, double& center, double& sigma, double& height);

    /// Create a Workspace2D for fitted peaks (pattern)
    DataObjects::Workspace2D_sptr genOutputFittedPatternWorkspace(std::vector<double> pattern, int workspaceindex);

    /// Calcualte the value of a single peak in a given range.
    void calculateSinglePeak(BackToBackExponential_sptr peak, BackgroundFunction_sptr background);

    /// Parse the fitting result
    std::string parseFitResult(API::IAlgorithm_sptr fitalg, double& chi2);

    // double calculateDspaceValue(std::vector<int> hkl);

    /// Calculate a Bragg peak's centre in TOF from its Miller indices and with instrumental parameters
    double calculatePeakCentreTOF(int h, int k, int l);

    /// Get parameter value from m_instrumentParameters
    double getParameter(string parname);

    /// Data
    API::MatrixWorkspace_sptr dataWS;

    /// Map for all peaks to fit individually
    std::map<std::vector<int>, CurveFitting::BackToBackExponential_sptr> mPeaks;

    /// Map for function (instrument parameter)
    std::map<std::string, double> m_instrumentParmaeters;

    /// Data for each individual peaks. (HKL)^2, vector index, function values
    std::vector<double> mPeakData;

    /// Peak parmeter names
    std::vector<std::string> mPeakParameterNames;

    /// TOF vector of data workspace to process with
    int workspaceindex;

    /// Flag to use given TOF in input table
    bool m_useGivenTOFh;

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


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_FITPOWDERDIFFPEAKS_H_ */
