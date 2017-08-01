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
  void singleFitComplete(bool error);
  void fitFunctionSelected(const QString &);
  void algorithmComplete(bool error);
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
  void updatePlotOptions();
  void addParametersToTree(const QStringList &parameters,
                           const QString &currentFitFunction);
  void addSampleLogsToWorkspace(const std::string &workspaceName,
                                const std::string &logName,
                                const std::string &logText,
                                const std::string &logType);
  void initFABADAOptions();
  void showFABADA(bool advanced);
  void hideFABADA();

  Ui::ConvFit m_uiForm;
  QtStringPropertyManager *m_stringManager;
  QtTreePropertyBrowser *m_cfTree;
  QMap<QtProperty *, QtProperty *> m_fixedProps;
  Mantid::API::MatrixWorkspace_sptr m_cfInputWS;
  Mantid::API::MatrixWorkspace_sptr m_previewPlotData;
  QString m_cfInputWSName;
  bool m_confitResFileType;
  Mantid::API::IAlgorithm_sptr m_singleFitAlg;
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
