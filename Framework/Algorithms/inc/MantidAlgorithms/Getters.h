#ifndef MANTID_ALGORITHMS_GETTERS_H_
#define MANTID_ALGORITHMS_GETTERS_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/GeometryInfo.h"

namespace Mantid {
namespace Algorithms {
namespace Getters {
static auto isMonitor = std::mem_fn(&GeometryInfo::isMonitor);
static auto isMasked = std::mem_fn(&GeometryInfo::isMasked);
static auto twoTheta = std::mem_fn(&GeometryInfo::getTwoTheta);
static auto constX = std::mem_fn(&API::MatrixWorkspace::readX);
static auto x = std::mem_fn(
    (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
    API::MatrixWorkspace::dataX);
static auto y = std::mem_fn(
    (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
    API::MatrixWorkspace::dataY);
static auto e = std::mem_fn(
    (std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
    API::MatrixWorkspace::dataE);
}
}
}

#endif /*MANTID_ALGORITHMS_GETTERS_H_*/
