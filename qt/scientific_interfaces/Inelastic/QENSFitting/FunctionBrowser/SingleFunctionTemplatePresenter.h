// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../ParameterEstimation.h"
#include "DllConfig.h"
#include "FunctionTemplatePresenter.h"
#include "SingleFunctionTemplateModel.h"

#include <QMap>

class QtProperty;

namespace MantidQt {
namespace MantidWidgets {
class EditLocalParameterDialog;
}
namespace CustomInterfaces {
namespace Inelastic {

class SingleFunctionTemplateView;

class MANTIDQT_INELASTIC_DLL SingleFunctionTemplatePresenter : public FunctionTemplatePresenter {
public:
  using FunctionTemplatePresenter::updateMultiDatasetParameters;

  explicit SingleFunctionTemplatePresenter(SingleFunctionTemplateView *view,
                                           std::unique_ptr<SingleFunctionTemplateModel> model);

  SingleFunctionTemplateView *view() const;
  SingleFunctionTemplateModel *model() const;

  void init() override;
  void updateAvailableFunctions(const std::map<std::string, std::string> &functionInitialisationStrings) override;

  void setFitType(std::string const &name) override;

  void setFunction(std::string const &funStr) override;

  EstimationDataSelector getEstimationDataSelector() const override;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data) override;
  void estimateFunctionParameters() override;

protected:
  void updateView() override;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
