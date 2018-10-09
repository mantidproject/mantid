// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MATRIXWORKSPACEACCESS_H_ */
