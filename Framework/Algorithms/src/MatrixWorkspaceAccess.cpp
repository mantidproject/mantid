#include "MantidAlgorithms/MatrixWorkspaceAccess.h"

namespace Mantid {
namespace Algorithms {

decltype(std::mem_fn(
    (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
    API::MatrixWorkspace::dataX)) MatrixWorkspaceAccess::x =
    std::mem_fn(
        (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
        API::MatrixWorkspace::dataX);
}
}
