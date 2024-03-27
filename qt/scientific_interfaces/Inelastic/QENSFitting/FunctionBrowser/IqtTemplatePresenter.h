// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Analysis/ParameterEstimation.h"
#include "DllConfig.h"
#include "FunctionTemplatePresenter.h"
#include "IqtFunctionTemplateModel.h"

class QtProperty;

namespace MantidQt {
namespace MantidWidgets {
class EditLocalParameterDialog;
}
namespace CustomInterfaces {
namespace IDA {

class IqtFunctionTemplateView;

class MANTIDQT_INELASTIC_DLL IqtTemplatePresenter : public FunctionTemplatePresenter {
public:
  explicit IqtTemplatePresenter(IqtFunctionTemplateView *view, std::unique_ptr<IqtFunctionTemplateModel> model);

  IqtFunctionTemplateView *view() const;
  IqtFunctionTemplateModel *model() const;

  void setNumberOfExponentials(int) override;
  void setStretchExponential(bool) override;
  void setBackground(std::string const &name) override;

  void setFunction(std::string const &funStr) override;

  void tieIntensities(bool on) override;
  bool canTieIntensities() const override;

  EstimationDataSelector getEstimationDataSelector() const override;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data) override;
  void estimateFunctionParameters() override;

protected:
  void updateView() override;

private:
  void updateViewParameters();
  void updateViewParameterNames();
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
