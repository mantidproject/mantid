#ifndef MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_

#include "IndirectDataAnalysisTab.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitAnalysisTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  /// Constructor
  IndirectFitAnalysisTab(QWidget *parent = nullptr);

protected:
  void setFitFunctions(const QVector<QString> &fitFunctions);

  void setPropertyFunctions(const QVector<QString> &functions);

  void setDefaultPropertyValue(const QString &propertyName,
                               const double &propertyValue);

  void removeDefaultPropertyValue(const QString &propertyName);

  bool hasDefaultPropertyValue(const QString &propertyName);

  bool hasParameterValue(const QString &propertyName,
                         const size_t &spectrumNumber);

  void fitAlgorithmComplete(const std::string &paramWSName);

  void fitAlgorithmComplete(const std::string &paramWSName,
                            const QHash<QString, QString> &propertyToParameter);

  QtProperty *createFunctionProperty(const QString &functionName,
                                     const bool &addParameters = true);

  QtProperty *createFunctionProperty(QtProperty *functionGroup,
                                     const bool &addParameters = true);

  QVector<QVector<QString>>
  getFunctionParameters(const QVector<QString> &functionNames) const;

  QVector<QString> getFunctionParameters(const QString &functionName) const;

  virtual Mantid::API::IFunction_sptr
  getFunction(const QString &functionName) const;

  void fixSelectedItem();

  void unFixSelectedItem();

  bool isFixable(QtProperty const *prop) const;

  bool isFixed(QtProperty const *prop) const;

  void fitContextMenu(const QString &menuName);

  void saveResult(const std::string &resultName);

  void plotResult(const std::string &resultName, const QString &plotType);

  void fillPlotTypeComboBox(QComboBox *comboBox);

  void
  updatePlot(const std::string &workspaceName,
             MantidQt::MantidWidgets::PreviewPlot *fitPreviewPlot,
             MantidQt::MantidWidgets::PreviewPlot *diffPreviewPlot) override;

  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  QtTreePropertyBrowser *m_propertyTree;

protected slots:
  virtual void algorithmComplete(bool error) = 0;

  void updateProperties(int specNo);

  void newInputDataLoaded(const QString &wsName);

  void clearBatchRunnerSlots();

  virtual void updatePreviewPlots() = 0;

private:
  /// Overidden by child class.
  void setup() override = 0;
  void run() override = 0;
  bool validate() override = 0;
  void loadSettings(const QSettings &settings) override = 0;
  virtual void disablePlotGuess() = 0;
  virtual void enablePlotGuess() = 0;

  /// Can be overidden by child class.
  virtual QString addPrefixToParameter(const QString &parameter,
                                       const QString &functionName,
                                       const int &functionNumber) const;
  virtual QString addPrefixToParameter(const QString &parameter,
                                       const QString &functionName) const;

  QVector<QVector<QString>>
  addPrefixToParameters(const QVector<QVector<QString>> &parameters,
                        const QVector<QString> &functionNames) const;

  QVector<QString> addPrefixToParameters(const QVector<QString> &parameters,
                                         const QString &functionName) const;

  QHash<QString, QString>
  createPropertyToParameterMap(const QVector<QString> &functionNames) const;

  QHash<QString, QString> createPropertyToParameterMap(
      const QVector<QString> &functionNames,
      const QVector<QVector<QString>> &parameters,
      const QVector<QVector<QString>> &parametersWithPrefix) const;

  QHash<QString, QString> createPropertyToParameterMap(
      const QString &functionName, const QVector<QString> &parameters,
      const QVector<QString> &parametersWithPrefix) const;

  QHash<QString, QHash<size_t, double>> combineParameterValues(
      const QHash<QString, QHash<size_t, double>> &parameterValues1,
      const QHash<QString, QHash<size_t, double>> &parameterValues2);

  void updateProperty(const QString &propertyName, const size_t &index);

  void clearFunctionProperties();

  QtStringPropertyManager *m_stringManager;
  QMap<QtProperty *, QtProperty *> m_fixedProps;
  QVector<QString> m_fitFunctions;
  QVector<QString> m_propertyFunctions;
  QHash<QString, QHash<size_t, double>> m_parameterValues;
  QHash<QString, QString> m_propertyToParameter;
  QMap<QString, double> m_defaultPropertyValues;
  bool m_appendResults;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_ */
