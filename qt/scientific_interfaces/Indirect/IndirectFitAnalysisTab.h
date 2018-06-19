#ifndef MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_

#include "IndirectDataAnalysisTab.h"
#include "IndirectFittingModel.h"
#include "IndirectSpectrumSelectionPresenter.h"
#include "IndirectSpectrumSelectionView.h"

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
  virtual void currentFinished() = 0;
};

template <typename Callback>
class DLLExport QtLazyAsyncRunner : public QtLazyAsyncRunnerBase {
public:
  using ReturnType = typename std::result_of<Callback()>::type;

  explicit QtLazyAsyncRunner()
      : m_current(), m_next(boost::none), m_initialized(false) {
    connect(&m_current, SIGNAL(finished()), this, SLOT(currentFinishedBase()));
  }

  void addCallback(Callback &&callback) {
    if (m_next.is_initialized())
      m_next = boost::none;

    if (m_current.isFinished() || !m_initialized)
      m_current.setFuture(QtConcurrent::run(callback));
    else
      m_next = std::forward<Callback>(callback);
    m_initialized = true;
  }

  bool isFinished() const { return m_current.isFinished(); }

  ReturnType result() const { return m_current.result(); }

protected:
  void currentFinished() override {
    if (m_next.is_initialized()) {
      m_current.setFuture(QtConcurrent::run(*m_next));
      m_next = boost::none;
      emit finished();
    } else
      emit finishedLazy();
  }

private:
  QFutureWatcher<ReturnType> m_current;
  boost::optional<Callback> m_next;
  bool m_initialized;
};

class DLLExport IndirectFitAnalysisTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  /// Constructor
  IndirectFitAnalysisTab(IndirectFittingModel *model,
                         QWidget *parent = nullptr);

  void setSpectrumSelectionView(IndirectSpectrumSelectionView *view);
  void
  setFitPropertyBrowser(MantidWidgets::IndirectFitPropertyBrowser *browser);

  Mantid::API::IFunction_sptr background() const;

  Mantid::API::IFunction_sptr model() const;

  boost::optional<size_t> backgroundIndex() const;

  QString selectedFitType() const;

  size_t numberOfCustomFunctions(const std::string &functionName) const;

  double startX() const;

  double endX() const;

  std::vector<double> parameterValue(const std::string &functionName,
                                     const std::string &parameterName) const;
  boost::optional<double>
  lastParameterValue(const std::string &functionName,
                     const std::string &parameterName) const;

  bool isEmptyModel() const;

  QString backgroundName() const;

  virtual bool canPlotGuess() const;

  virtual bool doPlotGuess() const = 0;

  void setConvolveMembers(bool convolveMembers);

  void updateTies();

  void setCustomSettingEnabled(const QString &customName, bool enabled);

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

  void setCustomSettingChangesFunction(const QString &settingKey,
                                       bool changesFunction);

protected:
  IndirectFittingModel *fittingModel() const;

  void plotResult(const QString &plotType);

  void fillPlotTypeComboBox(QComboBox *comboBox);

  void updatePlots(MantidWidgets::PreviewPlot *fitPreviewPlot,
                   MantidWidgets::PreviewPlot *diffPreviewPlot);

  void setAlgorithmProperties(Mantid::API::IAlgorithm_sptr fitAlgorithm) const;
  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  void runSingleFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  virtual void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  virtual void addGuessPlot(Mantid::API::MatrixWorkspace_sptr workspace) = 0;
  virtual void removeGuessPlot() = 0;
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

  Mantid::API::MatrixWorkspace_sptr
  createGuessWorkspace(Mantid::API::IFunction_const_sptr func,
                       int wsIndex) const;

  std::vector<double> computeOutput(Mantid::API::IFunction_const_sptr func,
                                    const std::vector<double> &dataX) const;

  void updatePlotOptions(QComboBox *cbPlotType);

  void setPlotOptions(QComboBox *cbPlotType,
                      const std::vector<std::string> &parameters) const;

  void setPlotOptions(QComboBox *cbPlotType,
                      const QSet<QString> &options) const;

  virtual void setPlotResultEnabled(bool enabled) = 0;
  virtual void setSaveResultEnabled(bool enabled) = 0;
  virtual void enablePlotPreview() = 0;
  virtual void disablePlotPreview() = 0;

  UserInputValidator &validateTab(UserInputValidator &validator);

signals:
  void functionChanged();

  void parameterChanged(const Mantid::API::IFunction *);

  void customBoolChanged(const QString &key, bool value);

protected slots:
  void setModelFitFunction();
  void setModelStartX(double startX);
  void setModelEndX(double endX);

  void updateFitOutput(bool error);
  void updateSingleFitOutput(bool error);
  void fitAlgorithmComplete(bool error);

  void clearGuessWindowPlot();

  void setSelectedSpectrum(int spectrum) override;

  virtual void startXChanged(double startX) = 0;

  virtual void endXChanged(double endX) = 0;

  void xMinSelected(double xMin);

  void xMaxSelected(double xMax);

  virtual void updatePlotRange() = 0;

  void singleFit();

  void executeFit();

  void newInputDataLoaded(const QString &wsName);

  void updateParameterValues();

  void updateParameterValues(
      const std::unordered_map<std::string, ParameterValue> &parameters);

  virtual void updatePreviewPlots() = 0;

  void updateGuessPlots();

  void updatePlotGuess();

  void updatePlotGuessInWindow();

  void plotGuessInWindow();

  virtual void updatePlotOptions() = 0;

  void emitFunctionChanged();

  void emitParameterChanged(const Mantid::API::IFunction *);

  void emitCustomBoolChanged(const QString &key, bool value);

  void updateResultOptions();

  void saveResult();

private:
  /// Overidden by child class.
  void setup() override;
  virtual void setupFitTab() = 0;
  void run() override;
  void loadSettings(const QSettings &settings) override = 0;
  virtual void disablePlotGuess() = 0;
  virtual void enablePlotGuess() = 0;

  void
  ensureAppendCompatibility(Mantid::API::MatrixWorkspace_sptr inputWS,
                            Mantid::API::MatrixWorkspace_sptr spectraWS) const;

  Mantid::API::IAlgorithm_sptr
  createWorkspaceAlgorithm(const std::string &workspaceName, int numSpec,
                           const std::vector<double> &dataX,
                           const std::vector<double> &dataY) const;
  Mantid::API::MatrixWorkspace_sptr
  extractSpectra(Mantid::API::MatrixWorkspace_sptr inputWS, int startIndex,
                 int endIndex, double startX, double endX) const;
  Mantid::API::MatrixWorkspace_sptr
  appendSpectra(Mantid::API::MatrixWorkspace_sptr inputWS,
                Mantid::API::MatrixWorkspace_sptr spectraWS) const;
  Mantid::API::MatrixWorkspace_sptr
  cropWorkspace(Mantid::API::MatrixWorkspace_sptr inputWS, double startX,
                double endX, int startIndex, int endIndex) const;
  void deleteWorkspace(Mantid::API::MatrixWorkspace_sptr workspace) const;

  Mantid::API::MatrixWorkspace_sptr m_inputAndGuessWorkspace;
  QtLazyAsyncRunner<std::function<void()>> m_plotWindowGuessRunner;

  std::unique_ptr<IndirectFittingModel> m_fittingModel;
  MantidWidgets::IndirectFitPropertyBrowser *m_fitPropertyBrowser;
  std::unique_ptr<IndirectSpectrumSelectionPresenter> m_spectrumPresenter;

  Mantid::API::IAlgorithm_sptr m_fittingAlgorithm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_ */
