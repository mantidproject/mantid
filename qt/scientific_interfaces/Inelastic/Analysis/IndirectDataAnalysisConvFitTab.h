// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndirectDataAnalysisTab.h"
#include "ParameterEstimation.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_IndirectFitTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class MANTIDQT_INELASTIC_DLL IndirectDataAnalysisConvFitTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisConvFitTab(QWidget *parent = nullptr);

  std::string getTabName() const override { return "ConvFit"; }

  bool hasResolution() const override { return true; }

private:
  EstimationDataSelector getEstimationDataSelector() const override;
  void addDataToModel(IAddWorkspaceDialog const *dialog) override;
  std::string getFitTypeString() const override;

  std::unique_ptr<Ui::IndirectFitTab> m_uiForm;
  // ShortHand Naming for fit functions
  std::unordered_map<std::string, std::string> m_fitStrings;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
