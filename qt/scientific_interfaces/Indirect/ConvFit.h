#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_

#include "IndirectFitAnalysisTab.h"
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

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected slots:
  void algorithmComplete(bool error) override;
  void typeSelection(int index);
  void bgTypeSelection(int index);
  void newDataLoaded(const QString &wsName);
  void extendResolutionWorkspace();
  void updatePreviewPlots() override;
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
  void showTieCheckbox(QString);
  void fitFunctionSelected(int fitTypeIndex);
  void saveClicked();
  void plotClicked();

private:
  void disablePlotGuess() override;
  void enablePlotGuess() override;

  QString addPrefixToParameter(const QString &parameter,
                               const QString &functionName,
                               const int &functionNumber) const override;

  QString addPrefixToParameter(const QString &parameter,
                               const QString &functionName) const override;

  boost::shared_ptr<Mantid::API::CompositeFunction>
  createFunction(bool tieCentres = false, bool addQValues = false);
  double
  getInstrumentResolution(Mantid::API::MatrixWorkspace_sptr workspaceName);

  void createTemperatureCorrection(Mantid::API::CompositeFunction_sptr product);
  QString fitTypeString() const;
  QString backgroundString() const;
  QString minimizerString(QString outputName) const;
  QVector<QString> indexToFitFunctions(const int &fitTypeIndex,
                                       const bool &includeDelta = true) const;
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
  Mantid::API::IFunction_sptr
  getFunction(const QString &functionName) const override;

  Ui::ConvFit m_uiForm;
  QtTreePropertyBrowser *m_cfTree;
  bool m_confitResFileType;
  QString m_baseName;

  // ShortHand Naming for fit functions
  QStringList m_fitStrings;

  // Used in auto generating defaults for parameters
  void createDefaultParamsMap();

  bool m_usedTemperature;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
