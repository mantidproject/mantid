// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFitAnalysisTab.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_IndirectFitTab.h"

#include <memory>

namespace Mantid {
namespace API {
class IFunction;
class CompositeFunction;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectDataAnalysisIqtFitTab : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisIqtFitTab(QWidget *parent = nullptr);

  std::string getTabName() const override { return "IqtFit"; }

  bool hasResolution() const override { return false; }

private:
  std::string getFitTypeString() const override;
  EstimationDataSelector getEstimationDataSelector() const override;
  void addDataToModel(IAddWorkspaceDialog const *dialog) override;

  std::unique_ptr<Ui::IndirectFitTab> m_uiForm;
  QString m_tiedParameter;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
