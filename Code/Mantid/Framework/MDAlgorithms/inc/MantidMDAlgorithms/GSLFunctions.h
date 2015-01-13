#ifndef MANTID_MDAlgorithms_GSLFUNCTIONS_H_
#define MANTID_MDAlgorithms_GSLFUNCTIONS_H_
#include "MantidAPI/CompositeFunction.h"

namespace Mantid {
namespace MDAlgorithms {
double f_eval(double x, void *params);
double f_eval2(double x, void *params);
} // namespace MDAlgorithms
} // namespace Mantid

#endif /*MANTID_MDAlgorithms_GSLFUNCTIONS_H_*/
