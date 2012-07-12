#ifndef MANTID_ALGORITHMS_LEBAILFIT_H_
#define MANTID_ALGORITHMS_LEBAILFIT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidCurveFitting/LeBailFunction.h"
#include "MantidCurveFitting/ThermoNeutronBackToBackExpPV.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace CurveFitting
{
  /** LeBailFit : Algorithm to do LeBail fitting.
    
    @date 2012-06-14

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
    virtual const std::string name() const { return "LeBailFit";};
    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;};
    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction";}

    /// Get parameter value (fitted)
    double getFittedParameterValue(std::string parname);

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    // Implement abstract Algorithm methods
    void init();
    // Implement abstract Algorithm methods
    void exec();

    void importParametersTable();
    void importReflections();

    void calPeakHeights(std::vector<double>& peakheights, size_t workspaceindex);
    bool estimatePeakRange(size_t workspaceindex, double center, double fwhm, double& tof_center, double& tof_left, double& tof_right);
    void calPeaksIntensity(size_t wsindex, std::set<size_t> peakindices, std::vector<double> peakcenters,
        std::vector<std::pair<double, double> > peakboundaries, std::vector<std::pair<size_t, double> >& peakintensities);

    void setLeBailParameters(CurveFitting::LeBailFunction_sptr func);
    void setLeBailPeakParameters(CurveFitting::LeBailFunction_sptr func);
    void initLeBailPeakParameters(CurveFitting::LeBailFunction_sptr func);

    bool iterateFit(size_t wsindex);

    /// Get background polynomial fit
    void getBackground(size_t wsindex);
    /// Find minimum value in a region
    size_t findMinValue(const MantidVec& vecX, const MantidVec& vecY, double leftbound, double rightbound);
    /// Fit background
    void fitBackground(DataObjects::Workspace2D_sptr bkgdWS, size_t polyorder);

    void observePeak(double& peakposition, double& leftfwhm, double& rightfwhm, double& peakheight);

    size_t findNearest(const MantidVec& vec, double value);

    CurveFitting::LeBailFunction_sptr mLeBail;

    API::MatrixWorkspace_sptr dataWS;
    DataObjects::TableWorkspace_sptr parameterWS;
    DataObjects::TableWorkspace_sptr reflectionWS;

    std::map<std::string, std::pair<double, char> > mFuncParameters; // char = f: fit... = t: tie to value
    std::map<std::string, std::vector<double> > mFittedParameters; // after each iteration of fitting, this will be increased by one
    std::vector<std::vector<int> > mPeakHKLs;
    double mLattice;

  };


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_LEBAILFIT_H_ */
