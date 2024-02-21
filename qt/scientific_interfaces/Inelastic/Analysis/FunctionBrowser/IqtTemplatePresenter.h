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
#include "IqtFunctionModel.h"

class QtProperty;

namespace MantidQt {
namespace MantidWidgets {
class EditLocalParameterDialog;
}
namespace CustomInterfaces {
namespace IDA {

class IqtTemplateBrowser;

class MANTIDQT_INELASTIC_DLL IqtTemplatePresenter : public FunctionTemplatePresenter {
public:
  explicit IqtTemplatePresenter(IqtTemplateBrowser *view, std::unique_ptr<IqtFunctionModel> model);

  IqtTemplateBrowser *view() const;
  IqtFunctionModel *model() const;

  void setNumberOfExponentials(int) override;
  void setStretchExponential(bool) override;
  void setBackground(std::string const &name) override;

  void setFunction(std::string const &funStr) override;

  void setGlobalParameters(std::vector<std::string> const &globals) override;
  void setGlobal(std::string const &parameterName, bool on) override;

  void updateMultiDatasetParameters(const Mantid::API::IFunction &fun) override;
  void updateMultiDatasetParameters(const Mantid::API::ITableWorkspace &table) override;
  void updateParameters(const IFunction &fun) override;

  void setCurrentDataset(int i) override;

  void tieIntensities(bool on) override;
  bool canTieIntensities() const override;

  EstimationDataSelector getEstimationDataSelector() const override;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data) override;
  void estimateFunctionParameters() override;

  void setBackgroundA0(double value) override;

  void handleEditLocalParameterFinished(std::string const &parameterName, QList<double> const &values,
                                        QList<bool> const &fixes, QStringList const &ties,
                                        QStringList const &constraints) override;

private:
  void updateViewParameters();
  void updateViewParameterNames();
  void updateView();
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
