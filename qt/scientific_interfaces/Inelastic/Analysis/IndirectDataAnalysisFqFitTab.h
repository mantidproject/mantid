// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataAnalysisTab.h"

#include "IFQFitObserver.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/TextAxis.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class IDAFunctionParameterEstimation;

class MANTIDQT_INELASTIC_DLL IndirectDataAnalysisFqFitTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisFqFitTab(QWidget *parent = nullptr);

  std::string getTabName() const override { return "FQFit"; }

  bool hasResolution() const override { return false; }

private:
  EstimationDataSelector getEstimationDataSelector() const override;
  IDAFunctionParameterEstimation createParameterEstimation() const;
  void addDataToModel(IAddWorkspaceDialog const *dialog) override;
  void setActiveWorkspaceIDToCurrentWorkspace(IAddWorkspaceDialog const *dialog);
  void setModelSpectrum(int index, const std::string &paramType);
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
