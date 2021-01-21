// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <string>
#include <utility>

using namespace Mantid::API;
namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW PlotFitAnalysisPaneModel {

public:
  PlotFitAnalysisPaneModel();
  virtual ~PlotFitAnalysisPaneModel(){};
  virtual IFunction_sptr doFit(const std::string &wsName, const std::pair<double, double> &range,
                               const IFunction_sptr func);
  virtual IFunction_sptr calculateEstimate(const std::string &workspaceName, const std::pair<double, double> &range);

  virtual bool hasEstimate() const;

private:
  IFunction_sptr calculateEstimate(MatrixWorkspace_sptr &workspace, const std::pair<double, double> &range);

  IFunction_sptr m_estimateFunction;
};

} // namespace MantidWidgets
} // namespace MantidQt
