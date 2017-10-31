#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_

#include "IndirectDataAnalysisTab.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_ConvFit.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport ConvFit : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  ConvFit(QWidget *parent = nullptr);

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

private slots:
  void typeSelection(int index);
  void bgTypeSelection(int index);
  void newDataLoaded(const QString wsName);
  void extendResolutionWorkspace();
  void updatePlot();
  void updatePlotRange();
  void plotGuess();
  void singleFit();
  void specMinChanged(int value);
  void specMaxChanged(int value);
  void minChanged(double);
  void maxChanged(double);
  void backgLevel(double);
  void updateRS(QtProperty *, double);
  void checkBoxUpdate(QtProperty *, bool);
  void hwhmChanged(double);
  void hwhmUpdateRS(double);
  void fitContextMenu(const QPoint &);
  void fixItem();
  void unFixItem();
  void showTieCheckbox(QString);
  void sequentialFitComplete(bool error);
  void singleFitComplete(bool error);
  void fitFunctionSelected(int fitTypeIndex);
  void saveClicked();
  void plotClicked();
  void updateProperties(int specNo);
  void addDefaultParametersToTree(const QString &fitFunction);

private:
  boost::shared_ptr<Mantid::API::CompositeFunction>
  createFunction(bool tieCentres = false);
  double
  getInstrumentResolution(Mantid::API::MatrixWorkspace_sptr workspaceName);
  QtProperty *createFitType(const QString &);
  QtProperty *createFitType(QtProperty *, const bool & = true);

  void createTemperatureCorrection(Mantid::API::CompositeFunction_sptr product);
  void populateFunction(Mantid::API::IFunction_sptr func,
                        Mantid::API::IFunction_sptr comp, QtProperty *group,
                        const std::string &pref, bool tie);
  QString fitTypeString() const;
  QString backgroundString() const;
  QString minimizerString(QString outputName) const;
  QVector<QString> getFunctionParameters(QString) const;
  QVector<QString> indexToFitFunctions(const int &fitTypeIndex);
  void updateProperties(int specNo, const QString &fitFunction);
  void updatePlotOptions();
  void addDefaultParametersToTree(const QVector<QString> &currentFitFunction);
  void addSampleLogsToWorkspace(const std::string &workspaceName,
                                const std::string &logName,
                                const std::string &logText,
                                const std::string &logType);
  void initFABADAOptions();
  void showFABADA(bool advanced);
  void hideFABADA();
  Mantid::API::IAlgorithm_sptr sequentialFit(const std::string &specMin,
                                             const std::string &specMax,
                                             QString &outputWSName);
  void algorithmComplete(bool error, const QString &outputWSName);
  QHash<QString, QString>
  createPropertyToParameterMap(const QVector<QString> &functionNames,
                               const QString &prefixPrefix,
                               const QString &prefixSuffix);
  void extendPropertyToParameterMap(
      const QString &functionName, const int &funcIndex,
      const QString &prefixPrefix, const QString &prefixSuffix,
      QHash<QString, QString> &propertyToParameter);
  void
  extendPropertyToParameterMap(const QString &functionName,
                               const QString &prefix,
                               QHash<QString, QString> &propertyToParameter);

  Ui::ConvFit m_uiForm;
  QtStringPropertyManager *m_stringManager;
  QtTreePropertyBrowser *m_cfTree;
  QMap<QtProperty *, QtProperty *> m_fixedProps;
  bool m_confitResFileType;
  QString m_baseName;

  // ShortHand Naming for fit functions
  QStringList m_fitStrings;

  // Used in auto generating defaults for parameters
  QMap<QString, double> m_defaultParams;
  QMap<QString, double> createDefaultParamsMap(QMap<QString, double> map);

  QVector<QString> m_fitFunctions;
  QHash<QString, QHash<size_t, double>> m_parameterValues;
  QHash<QString, QString> m_propertyToParameter;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
