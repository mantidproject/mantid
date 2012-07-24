#ifndef MANTID_CURVEFITTING_LEBAILFIT2_H_
#define MANTID_CURVEFITTING_LEBAILFIT2_H_

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

  /** LeBailFit2 : Algorithm to do Le Bail Fit.
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
  class DLLExport LeBailFit2 : public API::Algorithm
  {
  public:
    LeBailFit2();
    virtual ~LeBailFit2();

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


    API::MatrixWorkspace_sptr dataWS;
    DataObjects::TableWorkspace_sptr parameterWS;
    DataObjects::TableWorkspace_sptr reflectionWS;

    std::vector<std::vector<int> > mPeakHKLs;
    API::CompositeFunction_sptr mLeBailFunction;

    std::map<std::string, std::pair<double, char> > mFuncParameters; // char = f: fit... = t: tie to value

    std::vector<CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr> mPeaks;
    
  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_LEBAILFIT2_H_ */
