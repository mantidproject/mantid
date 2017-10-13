#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_

#include "IndirectDataAnalysisTab.h"
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

class DLLExport IqtFit : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IqtFit(QWidget *parent = nullptr);

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

private slots:
  void typeSelection(int index);
  void newDataLoaded(const QString wsName);
  void updatePlot();
  void specMinChanged(int value);
  void specMaxChanged(int value);
  void xMinSelected(double val);
  void xMaxSelected(double val);
  void backgroundSelected(double val);
  void propertyChanged(QtProperty *, double);
  void checkBoxUpdate(QtProperty *prop, bool checked);
  void updateCurrentPlotOption(QString newOption);
  void singleFit();
  void updateGuessPlot();
  void plotGuess(QtProperty *);
  void fitContextMenu(const QPoint &);
  void fixItem();
  void unFixItem();
  void singleFitComplete(bool error);
  void algorithmComplete(bool error);
  void plotWorkspace();
  void saveResult();
  void plotCurrentPreview();
  void updateProperties(int specNo);

private:
  boost::shared_ptr<Mantid::API::CompositeFunction>
  createFunction(bool tie = false);
  boost::shared_ptr<Mantid::API::IFunction>
  createExponentialFunction(const QString &name, bool tie = false);
  QtProperty *createExponential(const QString &);
  QtProperty *createStretchedExp(const QString &);
  void setDefaultParameters(const QString &name);
  QString fitTypeString() const;
  void constrainIntensities(Mantid::API::CompositeFunction_sptr func);
  QString minimizerString(QString outputName) const;
  std::string constructBaseName(const std::string &inputName,
                                const std::string &fitType, const bool &multi,
                                const size_t &specMin, const size_t &specMax);
  Mantid::API::IAlgorithm_sptr
  iqtFitAlgorithm(Mantid::API::MatrixWorkspace_sptr inputWs,
                  const size_t &specMin, const size_t &specMax);
  void updateFitFunctions();
  void plotResult(const std::string &groupName, const size_t &specNo);
  void resizePlotRange(MantidQt::MantidWidgets::PreviewPlot *preview);
  QHash<QString, QString>
  createParameterToPropertyMap(const QVector<QString> &functionNames);
  void
  extendParameterToPropertyMap(const QString &functionName,
                               const QString &prefix,
                               QHash<QString, QString> &parameterToProperty);
  Mantid::API::IAlgorithm_sptr replaceInfinityAndNaN(const std::string &wsName);

  Ui::IqtFit m_uiForm;
  QtStringPropertyManager *m_stringManager;
  QtTreePropertyBrowser *m_iqtFTree;           ///< IqtFit Property Browser
  QtDoublePropertyManager *m_iqtFRangeManager; ///< StartX and EndX for IqtFit
  QMap<QtProperty *, QtProperty *> m_fixedProps;
  QString m_ties;
  Mantid::API::IAlgorithm_sptr m_singleFitAlg;
  QString m_singleFitOutputName;
  std::string m_plotOption;
  std::string m_baseName;
  size_t m_runMin;
  size_t m_runMax;

  QVector<QString> m_fitFunctions;
  QHash<QString, QHash<size_t, double>> m_parameterValues;
  QHash<QString, QString> m_parameterToProperty;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_ */
