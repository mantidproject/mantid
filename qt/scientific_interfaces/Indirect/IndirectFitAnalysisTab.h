#ifndef MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_

#include "IndirectDataAnalysisTab.h"

#include "MantidQtWidgets/Common/IndirectFitPropertyBrowser.h"

#include <boost/optional.hpp>

#include <QtCore>

#include <memory>
#include <type_traits>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport QtLazyAsyncRunnerBase : public QObject {
  Q_OBJECT

signals:
  void finished();
  void finishedLazy();

protected slots:
  void currentFinishedBase() { currentFinished(); }

protected:
  void emitFinished() { emit finished(); };
  void emitFinishedLazy() { emit finishedLazy(); };
  virtual void currentFinished() = 0;
};

template <typename Callback>
class DLLExport QtLazyAsyncRunner : public QtLazyAsyncRunnerBase {
public:
  using ReturnType = typename std::result_of<Callback()>::type;

  explicit QtLazyAsyncRunner() : m_current(), m_next(boost::none) {
    connect(&m_current, SIGNAL(finished()), this, SLOT(currentFinishedBase()));
  }

  void addCallback(Callback &&callback) {
    if (m_next.is_initialized())
      m_next = boost::none;

    if (m_current.isFinished())
      m_current.setFuture(QtConcurrent::run(callback));
    else
      m_next = std::forward<Callback>(callback);
  }

  bool isFinished() const { return m_current.isFinished(); }

  ReturnType result() const { return m_current.result(); }

protected:
  void currentFinished() override {
    if (m_next.is_initialized()) {
      m_current.setFuture(QtConcurrent::run(*m_next));
      m_next = boost::none;
      emitFinished();
    } else
      emitFinishedLazy();
  }

private:
  QFutureWatcher<ReturnType> m_current;
  boost::optional<Callback> m_next;
};

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

  virtual bool doPlotGuess() const = 0;

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

  void updateGuessPlots(Mantid::API::IFunction_sptr guessFunction);

  void updatePlotGuess(Mantid::API::MatrixWorkspace_sptr workspace);
  void updatePlotGuessInWindow(Mantid::API::MatrixWorkspace_sptr workspace);

  Mantid::API::MatrixWorkspace_sptr createInputAndGuessWorkspace(
      Mantid::API::MatrixWorkspace_sptr guessWorkspace);

  Mantid::API::MatrixWorkspace_sptr
  createInputAndGuessWorkspace(Mantid::API::MatrixWorkspace_sptr inputWS,
                               Mantid::API::MatrixWorkspace_sptr guessWorkspace,
                               const std::string &outputName) const;

  Mantid::API::MatrixWorkspace_sptr createInputAndGuessWorkspace(
      Mantid::API::MatrixWorkspace_sptr inputWS,
      Mantid::API::MatrixWorkspace_sptr guessWorkspace) const;

  virtual Mantid::API::IAlgorithm_sptr singleFitAlgorithm() const;

  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const;

  Mantid::API::MatrixWorkspace_sptr
  createGuessWorkspace(Mantid::API::IFunction_const_sptr func,
                       int wsIndex) const;

  std::vector<double> computeOutput(Mantid::API::IFunction_const_sptr func,
                                    const std::vector<double> &dataX) const;

  void updatePlotOptions(QComboBox *cbPlotType);

  void setPlotOptions(QComboBox *cbPlotType,
                      const std::vector<std::string> &parameters);

  virtual void setMaxIterations(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                                int maxIterations) const;

  virtual std::string createSequentialFitOutputName() const;

  virtual std::string createSingleFitOutputName() const = 0;

  virtual void addGuessPlot(Mantid::API::MatrixWorkspace_sptr workspace) = 0;
  virtual void removeGuessPlot() = 0;

  virtual void enablePlotResult() = 0;
  virtual void disablePlotResult() = 0;
  virtual void enableSaveResult() = 0;
  virtual void disableSaveResult() = 0;

signals:
  void functionChanged();

  void parameterChanged(const Mantid::API::IFunction *);

protected slots:
  void clearGuessWindowPlot();

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

  void updateGuessPlots();

  void updatePlotGuess();

  void updatePlotGuessInWindow();

  void plotGuessInWindow();

  void guessWorkspaceCreated();

  virtual void updatePlotOptions() = 0;

  void emitFunctionChanged();

  void emitParameterChanged(const Mantid::API::IFunction *);

  void updateResultOptions();

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

  void
  ensureAppendCompatibility(Mantid::API::MatrixWorkspace_sptr inputWS,
                            Mantid::API::MatrixWorkspace_sptr spectraWS) const;

  Mantid::API::IAlgorithm_sptr
  createWorkspaceAlgorithm(const std::string &workspaceName, int numSpec,
                           const std::vector<double> &dataX,
                           const std::vector<double> &dataY) const;
  Mantid::API::IAlgorithm_sptr
  extractSpectraAlgorithm(Mantid::API::MatrixWorkspace_sptr inputWS,
                          int startIndex, int endIndex, double startX,
                          double endX) const;
  Mantid::API::IAlgorithm_sptr
  appendSpectraAlgorithm(Mantid::API::MatrixWorkspace_sptr inputWS,
                         Mantid::API::MatrixWorkspace_sptr spectraWS) const;
  Mantid::API::IAlgorithm_sptr
  cropWorkspaceAlgorithm(Mantid::API::MatrixWorkspace_sptr inputWS,
                         double startX, double endX, int startIndex,
                         int endIndex) const;
  Mantid::API::IAlgorithm_sptr
  deleteWorkspaceAlgorithm(Mantid::API::MatrixWorkspace_sptr workspace) const;

  Mantid::API::IFunction_sptr m_fitFunction;
  QHash<size_t, QHash<QString, double>> m_parameterValues;
  QHash<QString, double> m_defaultPropertyValues;
  QHash<QString, QString> m_functionNameChanges;
  MantidWidgets::IndirectFitPropertyBrowser *m_fitPropertyBrowser;

  std::string m_outputFitName;
  bool m_appendResults;
  Mantid::API::MatrixWorkspace_sptr m_inputAndGuessWorkspace;

  QtLazyAsyncRunner<std::function<Mantid::API::MatrixWorkspace_sptr()>>
      m_createGuessRunner;
  QtLazyAsyncRunner<std::function<void()>> m_plotWindowGuessRunner;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_ */
