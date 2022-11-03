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

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW IPlotFitAnalysisPaneModel {

public:
  IPlotFitAnalysisPaneModel(){};
  virtual ~IPlotFitAnalysisPaneModel(){};

  virtual void doFit(const std::string &wsName, const std::pair<double, double> &range) = 0;
  virtual void calculateEstimate(const std::string &workspaceName, const std::pair<double, double> &range) = 0;

  virtual void setPeakCentre(const double centre) = 0;
  virtual double peakCentre() const = 0;

  virtual std::string fitStatus() const = 0;
};

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW PlotFitAnalysisPaneModel : public IPlotFitAnalysisPaneModel {

public:
  PlotFitAnalysisPaneModel();
  virtual ~PlotFitAnalysisPaneModel(){};
  void doFit(const std::string &wsName, const std::pair<double, double> &range) override;
  void calculateEstimate(const std::string &workspaceName, const std::pair<double, double> &range) override;

  void setPeakCentre(const double centre) override;
  double peakCentre() const override;

  std::string fitStatus() const override;

private:
  IFunction_sptr calculateEstimate(MatrixWorkspace_sptr &workspace, const std::pair<double, double> &range);

  IFunction_sptr m_function;
  std::string m_fitStatus;
};

} // namespace MantidWidgets
} // namespace MantidQt
