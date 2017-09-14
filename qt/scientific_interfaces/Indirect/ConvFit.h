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
  ConvFit(QWidget *parent = 0);

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
  void plotGuess();
  void singleFit();
  void plotSpecChanged(int value);
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
  void fitFunctionSelected(const QString &);
  void saveClicked();
  void plotClicked();
  void plotCurrentPreview();

private:
  boost::shared_ptr<Mantid::API::CompositeFunction>
  createFunction(bool tieCentres = false);
  double getInstrumentResolution(std::string workspaceName);
  QtProperty *createFitType(const QString &);

  void createTemperatureCorrection(Mantid::API::CompositeFunction_sptr product);
  void populateFunction(Mantid::API::IFunction_sptr func,
                        Mantid::API::IFunction_sptr comp, QtProperty *group,
                        const std::string &pref, bool tie);
  QString fitTypeString() const;
  QString backgroundString() const;
  QString minimizerString(QString outputName) const;
  QStringList getFunctionParameters(QString);
  void updateParameters(int specNo);
  void updateParameters(const QString &functionName, const QString &prefix,
                        const QStringList &paramNames,
                        const QMap<QString, double> &paramValues,
                        int startOffset = 0, int endOffset = 0);
  void updatePlotOptions();
  void plotOutput(std::string const &outputWs, int specNo);
  void addParametersToTree(const QStringList &parameters,
                           const QString &currentFitFunction);
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

  Ui::ConvFit m_uiForm;
  QtStringPropertyManager *m_stringManager;
  QtTreePropertyBrowser *m_cfTree;
  QMap<QtProperty *, QtProperty *> m_fixedProps;
  // Pointer to sample workspace object
  Mantid::API::MatrixWorkspace_sptr m_cfInputWS;
  Mantid::API::MatrixWorkspace_sptr m_previewPlotData;
  Mantid::API::ITableWorkspace_sptr m_paramWs;
  QString m_cfInputWSName;
  int m_fittedIndex;
  bool m_confitResFileType;
  QString m_singleFitOutputName;
  QString m_previousFit;
  QString m_baseName;
  int m_runMin;
  int m_runMax;

  // ShortHand Naming for fit functions
  QStringList m_fitStrings;

  // Used in auto generating defaults for parameters
  QMap<QString, double> m_defaultParams;
  QMap<QString, double> createDefaultParamsMap(QMap<QString, double> map);
  QMap<QString, double>
  constructFullPropertyMap(const QMap<QString, double> &defaultMap,
                           const QStringList &parameters,
                           const QString &fitFunction);
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
