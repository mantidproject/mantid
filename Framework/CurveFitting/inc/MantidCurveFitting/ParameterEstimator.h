#ifndef MANTID_CURVEFITTING_PARAMETERESTIMATOR_H_
#define MANTID_CURVEFITTING_PARAMETERESTIMATOR_H_

#include "MantidKernel/System.h"

#include <vector>

namespace Mantid {

namespace API {
class IFunction;
class FunctionDomain1D;
class FunctionValues;
} // namespace API

namespace CurveFitting {
namespace ParameterEstimator {

/// ParameterEstimator estimates parameter values of some fitting functions
///  from fitting data.
void DLLExport estimate(API::IFunction &function,
                        const API::FunctionDomain1D &domain,
                        const API::FunctionValues &values);

} // namespace ParameterEstimator
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_PARAMETERESTIMATOR_H_ */
