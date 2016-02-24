#ifndef MANTID_ALGORITHMS_MATRIXWORKSPACEGETTERS_H_
#define MANTID_ALGORITHMS_MATRIXWORKSPACEGETTERS_H_

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {
namespace Getters {
static auto x = std::mem_fn(
    (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
    API::MatrixWorkspace::dataX);
}
}
}

#endif /* MANTID_ALGORITHMS_MATRIXWORKSPACEGETTERS_H_ */
