#ifndef MANTID_ALGORITHMS_MATRIXWORKSPACEACCESS_H_
#define MANTID_ALGORITHMS_MATRIXWORKSPACEACCESS_H_

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

struct MatrixWorkspaceAccess {
  static decltype(std::mem_fn(
      (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
      API::MatrixWorkspace::dataX)) x;
};
}
}

#endif /* MANTID_ALGORITHMS_MATRIXWORKSPACEACCESS_H_ */
