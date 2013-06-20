#include "MantidCurveFitting/ComptonScatteringCountRate.h"
#include "MantidCurveFitting/SimplexMinimizer.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Math/Optimization/SLSQPMinimizer.h"

namespace Mantid
{
namespace CurveFitting
{
  using Kernel::Math::SLSQPMinimizer;

  DECLARE_FUNCTION(ComptonScatteringCountRate)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ComptonScatteringCountRate::ComptonScatteringCountRate()
    : CompositeFunction(), m_profiles(), m_fixedParamIndices(), m_cmatrix(), 
    m_eqMatrix(), m_errors(), m_dataErrorRatio()
  {
  }

  //----------------------------------------------------------------------------------------------
  // Private methods
  //----------------------------------------------------------------------------------------------

  namespace
  {
    ///cond
    struct Norm2
    {
      /// Compute the value of the objective function
      Norm2(const Kernel::DblMatrix & cmatrix, const std::vector<double> & data)
        : cm(cmatrix), nrows(cmatrix.numRows()), ncols(cmatrix.numCols()), rhs(data) {}
      double eval(const std::vector<double> & xpt) const
      {
        double norm2(0.0);
        for(size_t i = 0; i < nrows; ++i)
        {
          const double *cmRow = cm[i];
          double cx(0.0);
          for(size_t j = 0; j < ncols; ++j)
          {
            cx += cmRow[j]*xpt[j];
          }
          cx -= rhs[i];
          norm2 += cx*cx;
        }
        return 0.5*norm2;
      }
      const Kernel::DblMatrix & cm;
      size_t nrows;
      size_t ncols;
      const std::vector<double> & rhs;
    };
    ///@endcond
  }

  /**
   * Calculates the new values for the intensity coefficents
   */
  void ComptonScatteringCountRate::iterationStarting()
  {
    /**
     * Before calling the functions that make up the composite perform a least-square minimization
     * to compute the values of the intensity coefficents so that they can be taken out of the fit
     * essentially. This also applies the intensity constraints supplied by the user.
     * The required parameters have been fixed by setupForFit
     *
     * It is required that we minimize 0.5*||Cx-d||^2, where C is a matrix of J*(y) values
     * from each intensity coefficient. This minimization is subject to the following constraints:
     *
     *   Cx >= 0, where C is the same matrix of J(y) values above
     *   Ax = 0, where A is the intensity constraints supplied by the user
     */
    const size_t nparams(m_cmatrix.numCols());

    // Compute minimization with of Cx where the amplitudes are set to 1.0 for J(y)
    std::vector<double> x0(nparams, 1);
    setFixedParameterValues(x0);    
    // Compute the constraint matrix
    this->updateCMatrixValues();

    Norm2 objfunc(m_cmatrix, m_dataErrorRatio);
    SLSQPMinimizer lsqmin(nparams, objfunc, m_eqMatrix, m_cmatrix);
    auto res = lsqmin.minimize(x0);

    // std::cerr << "\n\nParameters [";
    // for(size_t i = 0; i < nparams; ++i)
    // {
    //   std::cerr << res[i] << ",";
    // }
    // std::cerr << "]\n";
    
    // Set the parameters for the 'real' function calls
    setFixedParameterValues(res);
  }

  /**
   * Set the fixed parameters to the given values
   * @param values A new set of values to set
   */
  void ComptonScatteringCountRate::setFixedParameterValues(const std::vector<double> & values)
  {
    assert(values.size() == m_fixedParamIndices.size());
    
    const size_t nparams = values.size();
    for(size_t i = 0; i < nparams; ++i)
    {
      this->setParameter(m_fixedParamIndices[i], values[i], true);
    }    
  }

  /**
   * Fills the pre-allocated C matrix with the appropriate values
   */
  void ComptonScatteringCountRate::updateCMatrixValues() const
  {
    // -- Compute constraint matrix from each "member function" --
    const size_t nfuncs = this->nFunctions();
    for(size_t i = 0, start = 0; i < nfuncs; ++i)
    {
      auto *profile = m_profiles[i];
      const size_t numFilled = profile->fillConstraintMatrix(m_cmatrix,start,m_errors);
      start += numFilled;
    }
  }

  /**
   * Cache workspace reference. Expects a MatrixWorkspace
   * @param ws A shared_ptr to a Workspace
   */
  void ComptonScatteringCountRate::setWorkspace(boost::shared_ptr<const API::Workspace> ws)
  {
    CompositeFunction::setWorkspace(ws);

    auto matrix = boost::dynamic_pointer_cast<const API::MatrixWorkspace>(ws);
    if(!matrix)
    {
      throw std::invalid_argument("ComptonScatteringCountRate - Expected an object of type MatrixWorkspace, type=" + ws->id());
    }
    // Grab the workspace index - Assumes it's the same for all functions
    int wsIndex = this->getFunction(0)->getAttribute("WorkspaceIndex").asInt();
    const auto & values = matrix->readY(wsIndex);
    const auto & errors = matrix->readE(wsIndex);
    
    // Keep the errors for the constraint calculation
    m_errors = errors;
    m_dataErrorRatio.resize(errors.size());
    std::transform(values.begin(), values.end(), errors.begin(), m_dataErrorRatio.begin(), std::divides<double>());

    // Cache ptrs cast to ComptonProfile functions to that we can compute the constraint matrix
    const size_t nfuncs = this->nFunctions();
    m_profiles.resize(nfuncs);
    m_fixedParamIndices.reserve(nfuncs + 3); // guess at max size
    for(size_t i = 0; i < nfuncs; ++i)
    {
      auto func = this->getFunction(i);
      if( auto profile = boost::dynamic_pointer_cast<ComptonProfile>(func) )
      {
        m_profiles[i] = profile.get();
        const size_t paramsOffset = paramOffset(i);
        auto fixedParams = profile->intensityParameterIndices();
        for(size_t j = 0; j < fixedParams.size(); ++j)
        {
          const size_t indexOfFixed = paramsOffset + fixedParams[j];
          this->fix(indexOfFixed);
          m_fixedParamIndices.push_back(indexOfFixed);
        }
      }
      else
      { 
        std::ostringstream os;
        os << "ComptonScatteringCountRate - Invalid function member at index '" << i << "'. "
           << "All members must be of type ComptonProfile";
        throw std::runtime_error(os.str());
      }
    }
   
    size_t nColsCMatrix(m_fixedParamIndices.size());
    m_cmatrix = Kernel::DblMatrix(errors.size(),nColsCMatrix);
    m_eqMatrix = Kernel::DblMatrix(1, nColsCMatrix);
    m_eqMatrix[0][0] = 0.0;
    m_eqMatrix[0][1] = 0.0;
    m_eqMatrix[0][2] = 1.0;
    m_eqMatrix[0][3] = 0.0;
    m_eqMatrix[0][4] = -4.0;
  }

} // namespace CurveFitting
} // namespace Mantid
