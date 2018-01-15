#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_

#include "IndirectFitAnalysisTab.h"
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

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected slots:
  void typeSelection(int index);
  void newDataLoaded(const QString wsName);
  void updatePreviewPlots() override;
  void specMinChanged(int value);
  void specMaxChanged(int value);
  void xMinSelected(double val);
  void xMaxSelected(double val);
  void backgroundSelected(double val);
  void propertyChanged(QtProperty *, double);
  void checkBoxUpdate(QtProperty *prop, bool checked);
  void updateCurrentPlotOption(QString newOption);
  void singleFit();
  void plotGuess();
  void fitContextMenu(const QPoint &);
  void algorithmComplete(bool error) override;
  void plotWorkspace();
  void saveResult();

private:
  void disablePlotGuess() override;
  void enablePlotGuess() override;
  boost::shared_ptr<Mantid::API::CompositeFunction>
  createFunction(bool tie = false);
  void setDefaultParameters();
  QString fitTypeString() const;
  void constrainIntensities(Mantid::API::CompositeFunction_sptr func);
  QString minimizerString(QString outputName) const;
  std::string constructBaseName(const std::string &inputName,
                                const std::string &fitType, const bool &multi,
                                const size_t &specMin, const size_t &specMax);
  Mantid::API::IAlgorithm_sptr
  iqtFitAlgorithm(Mantid::API::MatrixWorkspace_sptr inputWs,
                  const size_t &specMin, const size_t &specMax);
  QVector<QString> indexToFitFunctions(const int &fitTypeIndex) const;
  Mantid::API::IAlgorithm_sptr replaceInfinityAndNaN(const std::string &wsName);
  Mantid::API::IFunction_sptr
  getFunction(const QString &functionName) const override;

  Ui::IqtFit m_uiForm;
  QtTreePropertyBrowser *m_iqtFTree; ///< IqtFit Property Browser
  QString m_ties;
  Mantid::API::IAlgorithm_sptr m_singleFitAlg;
  QString m_singleFitOutputName;
  std::string m_plotOption;
  std::string m_baseName;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_ */
