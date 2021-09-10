// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

struct MatrixWorkspaceAccess {
  static decltype(std::mem_fn((std::vector<double> & (API::MatrixWorkspace::*)(const std::size_t)) &
                              API::MatrixWorkspace::dataX)) x;
};
} // namespace Algorithms
} // namespace Mantid
