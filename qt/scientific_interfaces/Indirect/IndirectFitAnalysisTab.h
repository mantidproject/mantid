#ifndef MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_

#include "IndirectDataAnalysisTab.h"

#include "MantidQtWidgets/Common/IndirectFitPropertyBrowser.h"

#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitAnalysisTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  /// Constructor
  IndirectFitAnalysisTab(QWidget *parent = nullptr);

  Mantid::API::IFunction_sptr background() const;

  Mantid::API::IFunction_sptr model() const;

  int backgroundIndex() const;

  QString selectedFitType() const;

  size_t numberOfCustomFunctions(const std::string &functionName) const;

  double startX() const;

  double endX() const;

  double parameterValue(const std::string &functionName,
                        const std::string &parameterName);

  bool emptyModel() const;

  QString backgroundName() const;

  bool previousFitModelSelected() const;

  virtual bool canPlotGuess() const;

  const std::string &outputWorkspaceName() const;

  void setCustomSettingEnabled(const QString &customName, bool enabled);

  void moveCustomFunctionsToEnd();

  void setParameterValue(const std::string &functionName,
                         const std::string &parameterName, double value);

  void setDefaultPeakType(const std::string &function);

  void addCheckBoxFunctionGroup(
      const QString &groupName,
      const std::vector<Mantid::API::IFunction_sptr> &functions,
      bool defaultValue = false);

  void addSpinnerFunctionGroup(
      const QString &groupName,
      const std::vector<Mantid::API::IFunction_sptr> &functions,
      int minimum = 0, int maximum = 10, int defaultValue = 0);

  void addComboBoxFunctionGroup(
      const QString &groupName,
      const std::vector<Mantid::API::IFunction_sptr> &functions);

  void setBackgroundOptions(const QStringList &backgrounds);

  bool boolSettingValue(const QString &settingKey) const;

  void setCustomBoolSetting(const QString &settingKey, bool value);

  int intSettingValue(const QString &settingKey) const;

  double doubleSettingValue(const QString &settingKey) const;

  QString enumSettingValue(const QString &settingKey) const;

  void addBoolCustomSetting(const QString &settingKey,
                            const QString &settingName,
                            bool defaultValue = false);

  void addDoubleCustomSetting(const QString &settingKey,
                              const QString &settingName,
                              double defaultValue = 0);

  void addIntCustomSetting(const QString &settingKey,
                           const QString &settingName, int defaultValue = 0);

  void addEnumCustomSetting(const QString &settingKey,
                            const QString &settingName,
                            const QStringList &options);

  void addOptionalDoubleSetting(const QString &settingKey,
                                const QString &settingName,
                                const QString &optionKey,
                                const QString &optionName, bool enabled = false,
                                double defaultValue = 0);

  QHash<QString, double> fitParameterValues() const;

  virtual QHash<QString, double> createDefaultValues() const;

  QHash<QString, double> defaultParameterValues() const;

  QHash<QString, double> parameterValues() const;

  virtual Mantid::API::IFunction_sptr fitFunction() const;

  virtual QHash<QString, QString>
  functionNameChanges(Mantid::API::IFunction_sptr function) const;

  virtual Mantid::API::MatrixWorkspace_sptr fitWorkspace() const;

protected:
  /**
   * Adds a fit property browser to the specified Indirect Fit Analysis Tab.
   *
   * @param tab The indirect fit analysis tab to add the fit property browser
   *            to.
   */
  template <typename FitTab> void addPropertyBrowserToUI(FitTab tab) {
    tab->properties->addWidget(m_fitPropertyBrowser);
  }

  void setDefaultPropertyValue(const QString &propertyName,
                               const double &propertyValue);

  void removeDefaultPropertyValue(const QString &propertyName);

  bool hasDefaultPropertyValue(const QString &propertyName);

  void fitAlgorithmComplete(const std::string &paramWSName);

  void saveResult(const std::string &resultName);

  void plotResult(const std::string &resultName, const QString &plotType);

  void fillPlotTypeComboBox(QComboBox *comboBox);

  void updatePlot(const std::string &workspaceName,
                  MantidWidgets::PreviewPlot *fitPreviewPlot,
                  MantidWidgets::PreviewPlot *diffPreviewPlot) override;

  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  void plotGuess(MantidWidgets::PreviewPlot *previewPlot);

  virtual Mantid::API::IAlgorithm_sptr singleFitAlgorithm() const;

  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const;

  Mantid::API::MatrixWorkspace_sptr
  createGuessWorkspace(Mantid::API::IFunction_const_sptr func, size_t wsIndex);

  std::vector<double> computeOutput(Mantid::API::IFunction_const_sptr func,
                                    const std::vector<double> &dataX);

  Mantid::API::IAlgorithm_sptr
  createWorkspaceAlgorithm(const std::string &workspaceName, int numSpec,
                           const std::vector<double> &dataX,
                           const std::vector<double> &dataY);

  void updatePlotOptions(QComboBox *cbPlotType);

  virtual void setMaxIterations(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                                int maxIterations) const;

  virtual std::string createSequentialFitOutputName() const;

  virtual std::string createSingleFitOutputName() const = 0;

signals:
  void functionChanged();

  void parameterChanged(const Mantid::API::IFunction *);

protected slots:
  void setSelectedSpectrum(int spectrum) override;

  virtual void startXChanged(double startX) = 0;

  virtual void endXChanged(double endX) = 0;

  void xMinSelected(double xMin);

  void xMaxSelected(double xMax);

  virtual void updatePlotRange() = 0;

  void executeSingleFit();

  void executeSequentialFit();

  virtual void algorithmComplete(bool error) = 0;

  void newInputDataLoaded(const QString &wsName);

  void clearBatchRunnerSlots();

  void updateParameterValues();

  virtual void updatePreviewPlots() = 0;

  virtual void plotGuess() = 0;

  void updatePlotGuess();

  virtual void updatePlotOptions() = 0;

  void emitFunctionChanged();

  void emitParameterChanged(const Mantid::API::IFunction *);

private:
  /// Overidden by child class.
  void setup() override = 0;
  void run() override = 0;
  void loadSettings(const QSettings &settings) override = 0;
  virtual void disablePlotGuess() = 0;
  virtual void enablePlotGuess() = 0;
  QSet<QString> parameterNames();
  void updateParametersFromTable(const std::string &paramWSName);
  Mantid::API::IFunction_sptr
  updateFunctionTies(Mantid::API::IFunction_sptr function,
                     const QHash<QString, QString> &functionNameChanges) const;

  Mantid::API::IFunction_sptr m_fitFunction;
  QHash<size_t, QHash<QString, double>> m_parameterValues;
  QHash<QString, double> m_defaultPropertyValues;
  QHash<QString, QString> m_functionNameChanges;
  MantidWidgets::IndirectFitPropertyBrowser *m_fitPropertyBrowser;

  std::string m_outputFitName;
  bool m_appendResults;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_ */
