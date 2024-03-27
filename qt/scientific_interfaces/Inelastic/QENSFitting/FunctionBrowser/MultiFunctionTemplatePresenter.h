// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "FunctionTemplatePresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class MultiFunctionTemplateModel;
class MultiFunctionTemplateView;

class MANTIDQT_INELASTIC_DLL MultiFunctionTemplatePresenter : public FunctionTemplatePresenter {
public:
  explicit MultiFunctionTemplatePresenter(MultiFunctionTemplateView *view,
                                          std::unique_ptr<MultiFunctionTemplateModel> model);

  MultiFunctionTemplateView *view() const;
  MultiFunctionTemplateModel *model() const;

  void setSubType(size_t subTypeIndex, int typeIndex) override;

  void setFunction(std::string const &funStr) override;

  EstimationDataSelector getEstimationDataSelector() const override;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data) override;
  void estimateFunctionParameters() override;

protected:
  void updateView() override;

private:
  void updateViewParameters();
  void updateViewParameterNames();
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
