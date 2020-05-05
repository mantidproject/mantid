// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction.h"

#include <map>
#include <string>
using namespace Mantid::API;
namespace MantidQt {
namespace MantidWidgets {

class PlotFitAnalysisPaneModel {

public:
  IFunction_sptr doFit(const std::string &wsName,
                       const std::pair<double, double> &range,
                       const IFunction_sptr &func);
};

} // namespace MantidWidgets
} // namespace MantidQt
