#ifndef MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_

#include "IndirectDataAnalysisTab.h"
#include <boost/weak_ptr.hpp>

class QwtPlotCurve;
class QwtPlot;
class QSettings;
class QString;

namespace MantidQt {
namespace MantidWidgets {
class RangeSelector;
}
} // namespace MantidQt

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitAnalysisTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  /// Constructor
  IndirectFitAnalysisTab(QWidget *parent = nullptr);

protected:
  void setDefaultPropertyValue(const QString &propertyName,
                               double propertyValue);

  void fitAlgorithmComplete(const std::string &paramWSName,
                            const QVector<QString> &functionNames,
                            const bool usedBackground = false);

  QVector<QVector<QString>>
  getFunctionParameters(QVector<QString> functionNames) const;

  QVector<QString> getFunctionParameters(QString functionName) const;

protected slots:
  void updateProperties(int specNo);

  virtual void newDataLoaded(const QString &wsName);

private:
  /// Overidden by child class.
  void setup() override = 0;
  /// Overidden by child class.
  void run() override = 0;
  /// Overidden by child class.
  bool validate() override = 0;
  /// Overidden by child class.
  virtual void loadSettings(const QSettings &settings) = 0;
  /// Can be overidden by child class.
  virtual QString addPrefixToParameter(const QString &parameter,
                                       const QString &functionName,
                                       const int &functionNumber) const;
  /// Can be overidden by child class.
  virtual QString addPrefixToParameter(const QString &parameter,
                                       const QString &functionName) const;
  /// Can be overidden by child class.
  QVector<QVector<QString>>
  addPrefixToParameters(const QVector<QVector<QString>> &parameters,
                        const QVector<QString> &functionNames) const;
  /// Can be overidden by child class.
  QVector<QString> addPrefixToParameters(const QVector<QString> &parameters,
                                         const QString &functionName) const;

  QHash<QString, QString> createPropertyToParameterMap(
      const QVector<QString> &functionNames,
      const QHash<QString, QString> &propertyToParameterMap) const;

  QHash<QString, QString> createPropertyToParameterMap(
      const QVector<QString> &functionNames,
      const QVector<QVector<QString>> &parameters,
      const QVector<QVector<QString>> &parametersWithPrefix) const;

  QHash<QString, QString> createPropertyToParameterMap(
      const QString &functionName, const QVector<QString> &parameters,
      const QVector<QString> &parametersWithPrefix) const;

  QHash<QString, QHash<size_t, double>> m_parameterValues;
  QHash<QString, QString> m_propertyToParameter;
  QMap<QString, double> m_defaultPropertyValues;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_ */
