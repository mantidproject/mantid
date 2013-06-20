#ifndef MANTID_CURVEFITTING_COMPTONSCATTERINGCOUNTRATE_H_
#define MANTID_CURVEFITTING_COMPTONSCATTERINGCOUNTRATE_H_

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/ComptonProfile.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
namespace CurveFitting
{

  /**
    Implements a specialized function that encapsulates the combination of ComptonProfile functions
    that give the Neutron count rate.
    
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
  class DLLExport ComptonScatteringCountRate : public API::CompositeFunction
  {
  public:
    /// Constructor
    ComptonScatteringCountRate();

  private:
    /// String identifier
    std::string name() const { return "ComptonScatteringCountRate"; }
    /// Set an attribute value (and possibly cache its value)
    void setAttribute(const std::string& name,const Attribute& value);
    /// Takes the string & constructs the constraint matrix
    void parseIntensityConstraintMatrix(const std::string & value);

    /// Called by the framework just before an iteration is starting
    void iterationStarting();
    /// Set the fixed parameters to the given values
    void setFixedParameterValues(const std::vector<double> & values);
    /// Refresh the values of the C matrix for this evaluation
    void updateCMatrixValues() const;

    /// Cache reference to workspace for use in setupForFit
    void setWorkspace(boost::shared_ptr<const API::Workspace> ws);
    /// Cache ptrs to the individial profiles
    void cacheComptonProfiles();
    /// Set up the constraint matrices
    void createConstraintMatrices();

    
    /// Holder for non-owning functions cast as ComptonProfiles
    std::vector<ComptonProfile*> m_profiles;
    /// Store parameter indices of intensity parameters that are fixed
    std::vector<size_t> m_fixedParamIndices;
    /// Positivity constraints on J(y)
    mutable Kernel::DblMatrix m_cmatrix;
    /// Intensity equality constraints
    Kernel::DblMatrix m_eqMatrix;
    /// Errors on the data
    std::vector<double>  m_errors;
    /// Ratio of data & errors
    std::vector<double> m_dataErrorRatio;
  };

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_COMPTONSCATTERINGCOUNTRATE_H_ */
