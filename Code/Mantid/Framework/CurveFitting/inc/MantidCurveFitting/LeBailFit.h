#ifndef MANTID_CURVEFITTING_LEBAILFIT_H_
#define MANTID_CURVEFITTING_LEBAILFIT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/ThermalNeutronBk2BkExpConvPV.h"
#include "MantidAPI/CompositeFunction.h"


using namespace Mantid;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace CurveFitting
{

  /** LeBailFit : Algorithm to do Le Bail Fit.
    The workflow and architecture of this algorithm is different from LeBailFit,
    though they hold the same interface to users.
    
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
  class DLLExport LeBailFit : public API::Algorithm
  {
  public:
    LeBailFit();
    virtual ~LeBailFit();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "LeBailFit";}

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

    /// Import peak parameters
    void importParametersTable();

    /// Import Miller Indices (HKL)
    void importReflections();

    /// Create a list of peaks
    void generatePeaksFromInput();

    /// Create and set up output table workspace for peaks
    void createPeaksWorkspace();

    /// Set parameters to each peak
    void setPeakParameters(CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak, double peakheight);

    /// Calcualte peak heights from model to data
    void calPeaksIntensities(std::vector<std::pair<int, double> >& peakheights, size_t workspaceindex);

    /// Calcualte peak intensities for single or overlapped peaks
    void calPerGroupPeaksIntensities(size_t wsindex, std::set<size_t> peakindices, std::vector<double> peakcenters,
                           std::vector<std::pair<double, double> > peakboundaries, std::vector<std::pair<size_t, double> >& peakintensities);

    /// Calculate LeBail pattern
    void calculatePattern(size_t workspaceindex);

    /// LeBailFit
    void doLeBailFit(size_t workspaceindex);

    /// Calculate Peaks' Intensities
    void calculatePeaksHeights(size_t workspaceindex);

    /// Estimate the range of a peak from observation
    bool observePeakRange(size_t workspaceindex, double center, double fwhm, double& tof_center, double& tof_left, double& tof_right);

    /// Numerically estimate the range of peak
    //  void estimatePeakRange(size_t workspaceindex, double center, double fwhm, double& tof_center, double& tof_left, double& tof_right);

    /// Instance data
    API::MatrixWorkspace_sptr dataWS;
    DataObjects::Workspace2D_sptr outputWS;
    DataObjects::TableWorkspace_sptr parameterWS;
    DataObjects::TableWorkspace_sptr reflectionWS;

    /// Peaks about input and etc.
    std::vector<int> mPeakHKL2; // Peak's h^2+k^2+l^2: seaving as key for mPeakHeights adn mPeaks
    std::vector<std::vector<int> > mPeakHKLs;
    std::map<int, double> mPeakHeights;
    std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr> mPeaks;

    API::CompositeFunction_sptr mLeBailFunction;
    std::map<std::string, std::pair<double, char> > mFuncParameters; // char = f: fit... = t: tie to value

  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_LEBAILFIT_H_ */
