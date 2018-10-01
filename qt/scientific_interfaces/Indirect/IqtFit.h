#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "IqtFitModel.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_IqtFit.h"

#include <boost/weak_ptr.hpp>

namespace Mantid {
namespace API {
class IFunction;
class CompositeFunction;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IqtFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  IqtFit(QWidget *parent = nullptr);

protected slots:
  void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;
  void updatePlotOptions() override;
  void fitFunctionChanged();
  void customBoolUpdated(const QString &key, bool value);
  void plotResult();
  void runClicked();

protected:
  bool shouldEnablePlotResult() override { return true; };

  void setPlotResultEnabled(bool enabled) override;
  void setSaveResultEnabled(bool enabled) override;

  void setRunIsRunning(bool running) override;

private:
  void setConstrainIntensitiesEnabled(bool enabled);
  std::string fitTypeString() const;

  void setupFitTab() override;

  void setRunEnabled(bool enabled);
  void setFitSingleSpectrumEnabled(bool enabled);

  void setPlotResultIsPlotting(bool plotting);

  IqtFitModel *m_iqtFittingModel;
  std::unique_ptr<Ui::IqtFit> m_uiForm;
  QString m_tiedParameter;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_ */
