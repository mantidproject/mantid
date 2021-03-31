// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/cow_ptr.h"
#include <functional>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

struct DataForParameterEstimation {
  std::vector<double> x;
  std::vector<double> y;
};

using DataForParameterEstimationCollection = std::vector<DataForParameterEstimation>;
using EstimationDataSelector = std::function<DataForParameterEstimation(
    const Mantid::MantidVec &x, const Mantid::MantidVec &y, const std::pair<double, double> range)>;

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
