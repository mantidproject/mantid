#include "MantidAlgorithms/MatrixWorkspaceAccess.h"

namespace Mantid {
namespace Algorithms {

///@cond Doxygen has problems for decltype for some reason.
/// Returns std::mem_fn object refering to MatrixWorkspace:dataX().
decltype(std::mem_fn(
    (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
    API::MatrixWorkspace::dataX)) MatrixWorkspaceAccess::x =
    std::mem_fn(
        (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
        API::MatrixWorkspace::dataX);
///@endcond
} // namespace Algorithms
} // namespace Mantid
