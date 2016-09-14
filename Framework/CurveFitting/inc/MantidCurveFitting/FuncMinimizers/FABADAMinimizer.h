#ifndef MANTID_CURVEFITTING_FABADAMINIMIZER_H_
#define MANTID_CURVEFITTING_FABADAMINIMIZER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"

#include <boost/random/normal_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {
/// Forward Declaration
class CostFuncLeastSquares;
}
}
}

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

/** FABADA : TODO: DESCRIPTION

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class DLLExport FABADAMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor
  FABADAMinimizer();
  /// Name of the minimizer.
  std::string name() const override { return "FABADA"; }
  /// Initialize minimizer, i.e. pass a function to minimize.
  void initialize(API::ICostFunction_sptr function,
                  size_t maxIterations) override;
  /// Do one iteration.
  bool iterate(size_t iter) override;
  /// Return current value of the cost function
  double costFunctionVal() override;
  /// Finalize minimization, eg store additional outputs
  void finalize() override;

private:
  //Forward declarations
  
  //REFACTORING
  ///Returns the step from a Gaussian given sigma = Jump
  double GaussianStep(const double& Jump);
  ///If the new point is out of its bounds, it is changed to fit in the bound limits
  void BoundApplication(const size_t& ParameterIndex, double& new_value, double& step);
  ///Applied to the other parameters first and sequentially, finally to the current one
  void TieApplication(const size_t& ParameterIndex, GSLVector& new_parameters, double& new_value);
  ///Given the new chi2, next position is calculated and updated. m_changes[ParameterIndex] updated too
  void AlgorithmDisplacement(const size_t& ParameterIndex, const double& chi2_new, GSLVector& new_parameters);
  ///Updates the ParameterIndex-th parameter jump if appropriate
  void JumpUpdate(const size_t& ParameterIndex);
  ///Check for convergence (including Overexploration convergence), updates m_converged
  void ConvergenceCheck();
  ///Refrigerates the system if appropriate
  void SimAnnealingRefrigeration();
  ///Decides wheather iteration must continue or not
  bool IterationContinuation();
  
  
  //Variables declarations
  /// Pointer to the cost function. Must be the least squares.
  // Intentar encontrar una manera de sacar aqui el numero de parametros  que
  // no sea necesaria la cost function
  boost::shared_ptr<CostFunctions::CostFuncLeastSquares> m_leastSquares;
  /// Pointer to the Fitting Function (IFunction) inside the cost function.
  API::IFunction_sptr m_FitFunction;
  /// The number of iterations done (restarted at each phase).
  size_t m_counter;
  /// The number of chain iterations
  size_t m_ChainIterations;
  /// The number of changes done on each parameter.
  std::vector<double> m_changes;
  /// The jump for each parameter
  std::vector<double> m_jump;
  /// Parameters' values.
  GSLVector m_parameters;
  /// Markov chain.
  std::vector<std::vector<double>> m_chain;
  /// The chi square result of previous iteration;
  double m_chi2;
  /// Boolean that indicates global convergence
  bool m_converged;
  /// The point when convergence has been reached
  size_t m_conv_point;
  /// Convergence of each parameter
  std::vector<bool> m_par_converged;
  /// Lower bound for each parameter
  std::vector<double> m_lower;
  /// Upper bound for each parameter
  std::vector<double> m_upper;
  /// Bool that indicates if there is any boundary constraint
  std::vector<bool> m_bound;
  /// Convergence criteria for each parameter
  std::vector<double> m_criteria;
  /// Maximum number of iterations
  size_t m_max_iter;
  ///Bool that idicates if a varible has changed at some self iteration
  std::vector<bool> m_par_changed;
  ///Simulated Annealing temperature
  double m_Temperature;
  ///The global number of iterations done
  size_t m_counterGlobal;
  ///Number of iterations between Simulated Annealing refrigeration points
  size_t m_SimAnnealingItStep;
  ///The number of refrigeration points left
  size_t m_LeftRefrPoints;
  ///Temperature step between different Simulated Annealing phases
  double m_TempStep;
  ///Overexploration applied
  bool m_Overexploration;
  ///Number of parameters of the FittingFunction (not necessarily the CostFunction)
  size_t m_nParams;
  ///Number of consecutive innactive regenerations needed to consider convergence
  size_t m_InnactConvCriterion;
  ///Number of consecutive regenerations without changes (>= m_InnactConvCriterion it is considered to have arrived to convergence at a steep minimum)
  std::vector<size_t> m_NumInactiveRegenerations;
  ///To track convergence through immobility
  std::vector<double> m_changesOld;
};

///Used to access the setDirty() protected member
class MaleableCostFunction: public CostFunctions::CostFuncLeastSquares {
	public:
	///To inform the main class of changes through the IFunction
	    void setDirtyInherited() {setDirty();}
};


} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_FABADAMINIMIZER_H_ */
