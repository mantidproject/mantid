#ifndef MANTID_CURVEFITTING_FABADAMINIMIZER_H_
#define MANTID_CURVEFITTING_FABADAMINIMIZER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include <boost/random/normal_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>

namespace Mantid {
namespace CurveFitting {
class CostFuncLeastSquares;

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
  virtual ~FABADAMinimizer();
  /// Name of the minimizer.
  std::string name() const { return "FABADA"; }
  /// Initialize minimizer, i.e. pass a function to minimize.
  virtual void initialize(API::ICostFunction_sptr function,
                          size_t maxIterations);
  /// Do one iteration.
  virtual bool iterate(size_t iter);
  /// Return current value of the cost function
  virtual double costFunctionVal();
  /// Finalize minimization, eg store additional outputs
  virtual void finalize();

private:
  /// Pointer to the cost function. Must be the least squares.
  /// Intentar encontrar una manera de sacar aqui el numero de parametros  que
  /// no sea necesaria la cost function
  boost::shared_ptr<CostFuncLeastSquares> m_leastSquares;
  /// The number of iterations done.
  size_t m_counter;
  ///
  size_t m_numberIterations;
  /// The number of changes done in each parameter.
  std::vector<double> m_changes;
  /// The jump for each parameter
  std::vector<double> m_jump;
  /// Parameters.
  GSLVector m_parameters;
  /// Markov chain.
  std::vector<std::vector<double>> m_chain;
  /// The chi square result of previous iteration;
  double m_chi2;
  /// Boolean that indicates if converged
  bool m_converged;
  /// The point when convergence starts
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
};
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_FABADAMINIMIZER_H_ */
