#ifndef MANTID_CURVEFITTING_REFINEPOWDERINSTRUMENTPARAMETERS_H_
#define MANTID_CURVEFITTING_REFINEPOWDERINSTRUMENTPARAMETERS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/ThermalNeutronBk2BkExpConvPV.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidCurveFitting/Bk2BkExpConvPV.h"
#include "MantidCurveFitting/Polynomial.h"

namespace Mantid
{
namespace CurveFitting
{

  /** RefinePowderInstrumentParameters : Algorithm to refine instrument geometry parameters only.
    How to use the algorithm to refine instrument geometry?
  - Input includes
  * Data workspace	dataWS
  * Peak parameters	?????  Not sure which to use.  (1) ThermalNeutronBk2BkExpConvPV OR (2) Bk2BkExpConvPV
  * List of peaks (H, K, L) to fit for Bk2BkExpConvPV
  - Algorithm
  * For each input peak:
    1. Calculate TOF_h
    2. Find a proper range of data including peak and background around each peak
    3. For each peak
      a) Remove peak and fit the background
         Considering: 	get a linear function of two ends
            only include certain range of data around this linear function
            fit for all the points selected
      b) Construct a composite function containing (1) Bk2BkExpConvPV and (2) background function
      c) Fix background and fit the peak
      d) If fitting result is acceptible, record TOF_h
      e) Record the calcualed data as further refer.
    4. Construct workspace peakpositionWS such that
       dataGeometry.X = d-spacing value
       dataGeometry.Y = Observed TOF_h
    5. Fit zero, zerot, Dtt1, Dtt1t,and/or Dtt2t

    
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport RefinePowderInstrumentParameters : public API::Algorithm
  {
  public:
    RefinePowderInstrumentParameters();
    virtual ~RefinePowderInstrumentParameters();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "RefinePowderInstrumentParameters";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 2;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    // Implement abstract Algorithm methods
    void init();
    // Implement abstract Algorithm methods
    void exec();

    void generatePeaksFromInput(DataObjects::TableWorkspace_sptr peakparamws,
                                std::map<std::vector<int>, CurveFitting::Bk2BkExpConvPV_sptr>& peaks);

    void importParametersFromTable(DataObjects::TableWorkspace_sptr parameterWS, std::map<std::string, double>& parameters);

    void fitPeaks(int workspaceindex, std::vector<std::vector<int> >& goodfitpeaks, std::vector<double> &goodfitchi2);

    /// Fit a single peak
    bool fitSinglePeak(API::CompositeFunction_sptr compfunction, Bk2BkExpConvPV_sptr peak, BackgroundFunction_sptr background,
                       double leftdev, double rightdev, size_t workspaceindex, double prog, double deltaprog,
                       double &chi2, std::string &fitstatus);

    bool fitSinglePeakSimple(API::CompositeFunction_sptr compfunction, CurveFitting::Bk2BkExpConvPV_sptr peak,
                             CurveFitting::BackgroundFunction_sptr background, double leftdev, double rightdev,
                             size_t workspaceindex, double prog, double deltaprog,
                             double& chi2, std::string& fitstatus);

    /// Find max height (peak center)
    void findMaxHeight(API::MatrixWorkspace_sptr dataws, size_t wsindex,
                       double xmin, double xmax, double& center, double& centerleftbound, double& centerrightbound);

    bool fitLinearBackground(CurveFitting::Bk2BkExpConvPV_sptr peak,
                             size_t workspaceindex, BackgroundFunction_sptr, double xmin, double xmax);

    /// Fit background by removing the peak analytically
    bool fitBackground(CurveFitting::Bk2BkExpConvPV_sptr peak, size_t workspaceindex,
                       CurveFitting::BackgroundFunction_sptr bkgdfunc, double xmin, double xmax);

    /// Generate (output) workspace of peak centers
    void genPeakCentersWorkspace(std::map<std::vector<int>, CurveFitting::Bk2BkExpConvPV_sptr> peaks,
                                      std::vector<std::vector<int> > goodpeaks, std::vector<double> goodfitchi2);

    /// Generate output peak parameters workspace
    void generateOutputPeakParameterWorkspace(std::vector<std::vector<int> > goodfitpeaks);

    /// Fit instrument geometry parameters by ThermalNeutronDtoTOFFunction
    void fitInstrumentParameters();

    /// Calculate d-space value from peak's miller index for thermal neutron
    double calculateDspaceValue(std::vector<int> hkl);

    /// Create output of peak patterns
    DataObjects::Workspace2D_sptr createPeakDataWorkspace(size_t workspaceindex);

    /// Data
    API::MatrixWorkspace_sptr dataWS;

    /// Output Workspace
    DataObjects::Workspace2D_sptr outWS;

    /// Map for all peaks to fit individually
    std::map<std::vector<int>, CurveFitting::Bk2BkExpConvPV_sptr> mPeaks;

    /// Map for function (instrument parameter)
    std::map<std::string, double> mFuncParameters;

    /// Map for input (original) function
    std::map<std::string, double> mInputFuncParameters;

    /// Peak centers in d-spacing and TOF for final fitting
    std::map<std::vector<int>, std::pair<double, double> > mPeakCenters;

    /// Data for each individual peaks. (HKL)^2, vector index, function values
    std::map<int, std::pair<std::vector<size_t>, API::FunctionValues> > mPeakData;

    ///
    std::vector<std::string> mPeakParameterNames;

  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_REFINEPOWDERINSTRUMENTPARAMETERS_H_ */
