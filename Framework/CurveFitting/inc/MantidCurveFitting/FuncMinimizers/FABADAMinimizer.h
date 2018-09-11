#ifndef MANTID_CURVEFITTING_FABADAMINIMIZER_H_
#define MANTID_CURVEFITTING_FABADAMINIMIZER_H_

#include "MantidAPI/IFuncMinimizer.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {
/// Forward Declaration
class CostFuncLeastSquares;
} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

/** FABADA : Implements the FABADA Algorithm, based on a Adaptive Metropolis
  Algorithm extended with Gibbs Sampling. Designed to obtain the Bayesian
  posterior PDFs

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

  /// Public methods only for testing purposes

  /// If the new point is out of its bounds, it is changed to fit in the bound
  /// limits
  void boundApplication(const size_t &parameterIndex, double &newValue,
                        double &step);

private:
  /// Returns the step from a Gaussian given sigma = Jump
  double gaussianStep(const double &jump);
  /// Applied to the other parameters first and sequentially, finally to the
  /// current one
  void tieApplication(const size_t &parameterIndex, GSLVector &newParameters,
                      double &newValue);
  /// Given the new chi2, next position is calculated and updated.
  /// m_changes[ParameterIndex] updated too
  void algorithmDisplacement(const size_t &parameterIndex,
                             const double &chi2New, GSLVector &newParameters);
  /// Updates the ParameterIndex-th parameter jump if appropriate
  void jumpUpdate(const size_t &parameterIndex);
  /// Check for convergence (including Overexploration convergence), updates
  /// m_converged
  void convergenceCheck();
  /// Refrigerates the system if appropriate
  void simAnnealingRefrigeration();
  /// Decides wheather iteration must continue or not
  bool iterationContinuation();
  /// Output Markov chains
  void outputChains();
  /// Output converged chains
  void outputConvergedChains(size_t convLength, int nSteps);
  /// Output cost function
  void outputCostFunctionTable(size_t convLength, double mostProbableChi2);
  /// Output PDF
  double outputPDF(size_t convLength,
                   std::vector<std::vector<double>> &reducedChain);
  /// Output parameter table
  void outputParameterTable(const std::vector<double> &bestParameters,
                            const std::vector<double> &errorsLeft,
                            const std::vector<double> &errorsRight);
  /// Calculated converged chain and parameters
  void calculateConvChainAndBestParameters(
      size_t convLength, int nSteps,
      std::vector<std::vector<double>> &reducedChain,
      std::vector<double> &bestParameters, std::vector<double> &errorLeft,
      std::vector<double> &errorRight);
  /// Initialize member variables related to fitting parameters
  void initChainsAndParameters();
  /// Initialize member variables related to simulated annealing
  void initSimulatedAnnealing();

  // Variables declarations
  /// Pointer to the cost function. Must be the least squares.
  // Intentar encontrar una manera de sacar aqui el numero de parametros  que
  // no sea necesaria la cost function
  boost::shared_ptr<CostFunctions::CostFuncLeastSquares> m_leastSquares;
  /// Pointer to the Fitting Function (IFunction) inside the cost function.
  API::IFunction_sptr m_fitFunction;
  /// The number of iterations done (restarted at each phase).
  size_t m_counter;
  /// The number of chain iterations
  size_t m_chainIterations;
  /// The number of changes done on each parameter.
  std::vector<int> m_changes;
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
  size_t m_convPoint;
  /// Convergence of each parameter
  std::vector<bool> m_parConverged;
  /// Convergence criteria for each parameter
  std::vector<double> m_criteria;
  /// Maximum number of iterations
  size_t m_maxIter;
  /// Bool that idicates if a varible has changed at some self iteration
  std::vector<bool> m_parChanged;
  /// Simulated Annealing temperature
  double m_temperature;
  /// The global number of iterations done
  size_t m_counterGlobal;
  /// Number of iterations between Simulated Annealing refrigeration points
  size_t m_simAnnealingItStep;
  /// The number of refrigeration points left
  size_t m_leftRefrPoints;
  /// Temperature step between different Simulated Annealing phases
  double m_tempStep;
  /// Overexploration applied
  bool m_overexploration;
  /// Number of parameters of the FittingFunction (not necessarily the
  /// CostFunction)
  size_t m_nParams;
  /// Number of consecutive regenerations without changes
  std::vector<size_t> m_numInactiveRegenerations;
  /// To track convergence through immobility
  std::vector<int> m_changesOld;
};

/// Used to access the setDirty() protected member
class MaleableCostFunction : public CostFunctions::CostFuncLeastSquares {
public:
  /// To inform the main class of changes through the IFunction
  void setDirtyInherited() { setDirty(); }
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_FABADAMINIMIZER_H_ */
