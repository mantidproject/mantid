#ifndef MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_

#include "IndirectDataAnalysisTab.h"

#include "MantidQtWidgets/Common/IndirectFitPropertyBrowser.h"

#include <boost/variant.hpp>

namespace Ui {
class IqtFit;
class ConvFit;
class MSDFit;
class JumpFit;
} // namespace Ui

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitAnalysisTab : public IndirectDataAnalysisTab {
  Q_OBJECT
  using UIForm =
      boost::variant<Ui::IqtFit *, Ui::ConvFit *, Ui::MSDFit *, Ui::JumpFit *>;

public:
  /// Constructor
  IndirectFitAnalysisTab(QWidget *parent = nullptr);

  Mantid::API::IFunction_sptr background() const;

  Mantid::API::IFunction_sptr model() const;

  size_t numberOfCustomFunctions(const std::string &functionName) const;

  double startX() const;

  double endX() const;

  double parameterValue(const std::string &functionName,
                        const std::string &parameterName);

  bool emptyFitFunction() const;

  const std::string backgroundName() const;

  void setParameterValue(const std::string &functionName,
                         const std::string &parameterName, double value);

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

  virtual Mantid::API::IFunction_sptr fitFunction() const;

  virtual Mantid::API::MatrixWorkspace_sptr fitWorkspace() const;

protected:
  void addPropertyBrowserToUI(UIForm form);

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

  virtual Mantid::API::IAlgorithm_sptr singleFitAlgorithm();

  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm();

  Mantid::API::MatrixWorkspace_sptr
  createGuessWorkspace(Mantid::API::IFunction_const_sptr func, size_t wsIndex);

  std::vector<double> computeOutput(Mantid::API::IFunction_const_sptr func,
                                    const std::vector<double> &dataX);

  Mantid::API::IAlgorithm_sptr
  createWorkspaceAlgorithm(const std::string &workspaceName, int numSpec,
                           const std::vector<double> &dataX,
                           const std::vector<double> &dataY);

protected slots:
  void setSelectedSpectrum(int spectrum) override;

  virtual void startXChanged(double startX) = 0;

  virtual void endXChanged(double endX) = 0;

  void xMinSelected(double xMin);

  void xMaxSelected(double xMax);

  virtual void rangeChanged(double xMin, double xMax) = 0;

  void executeSingleFit();

  void executeSequentialFit();

  virtual void algorithmComplete(bool error) = 0;

  void newInputDataLoaded(const QString &wsName);

  void clearBatchRunnerSlots();

  void fitFunctionChanged();

  virtual void updatePreviewPlots() = 0;

  virtual void plotGuess() = 0;

private:
  /// Overidden by child class.
  void setup() override = 0;
  void run() override = 0;
  void loadSettings(const QSettings &settings) override = 0;
  virtual void disablePlotGuess() = 0;
  virtual void enablePlotGuess() = 0;
  QSet<QString> parameterNames();

  Mantid::API::IFunction_sptr m_fitFunction;
  QHash<size_t, QHash<QString, double>> m_parameterValues;
  QHash<QString, double> m_defaultPropertyValues;
  MantidWidgets::IndirectFitPropertyBrowser *m_fitPropertyBrowser;
  bool m_appendResults;

  Mantid::API::MatrixWorkspace_sptr m_guessWorkspace;
  int m_guessSpectrum;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_ */
