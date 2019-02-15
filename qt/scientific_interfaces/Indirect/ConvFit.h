// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_

#include "ConvFitModel.h"
#include "IndirectFitAnalysisTab.h"
#include "IndirectSpectrumSelectionPresenter.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_ConvFit.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport ConvFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  ConvFit(QWidget *parent = nullptr);

  QStringList getSampleWSSuffices() const override;
  QStringList getSampleFBSuffices() const override;
  QStringList getResolutionWSSuffices() const override;
  QStringList getResolutionFBSuffices() const override;

protected slots:
  void setModelResolution(const QString &resolutionName);
  void runClicked();
  void fitFunctionChanged();

protected:
  void setRunIsRunning(bool running) override;
  void setRunEnabled(bool enable) override;

private:
  void setupFitTab() override;
  void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;

  std::string fitTypeString() const;

  std::unique_ptr<Ui::ConvFit> m_uiForm;
  // ShortHand Naming for fit functions
  QHash<QString, std::string> m_fitStrings;
  ConvFitModel *m_convFittingModel;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
