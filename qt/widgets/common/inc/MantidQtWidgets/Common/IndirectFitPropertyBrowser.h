#ifndef INDIRECTFITPROPERTYBROWSER_H_
#define INDIRECTFITPROPERTYBROWSER_H_

#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"

#include <QSet>

#include <unordered_map>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON IndirectFitPropertyBrowser
    : public FitPropertyBrowser {
  Q_OBJECT

public:
  /// Constructor.
  IndirectFitPropertyBrowser(QWidget *parent = nullptr,
                             QObject *mantidui = nullptr);
  /// Initialise the layout.
  void init() override;

  Mantid::API::IFunction_sptr background() const;

  int backgroundIndex() const;

  int functionIndex(Mantid::API::IFunction_sptr function) const;

  QString selectedFitType() const;

  QString backgroundName() const;

  size_t numberOfCustomFunctions(const std::string &functionName) const;

  double parameterValue(const std::string &functionName,
                        const std::string &parameterName) const;

  void setParameterValue(const std::string &functionName,
                         const std::string &parameterName, double value);

  void setParameterValue(Mantid::API::IFunction_sptr function,
                         const std::string &parameterName, double value);

  void setBackground(const std::string &backgroundName);

  void moveCustomFunctionsToEnd();

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

  void addIntCustomSetting(const QString &settingKey,
                           const QString &settingName, int defaultValue = 0);

  void addDoubleCustomSetting(const QString &settingKey,
                              const QString &settingName,
                              double defaultValue = 0);

  void addEnumCustomSetting(const QString &settingKey,
                            const QString &settingName,
                            const QStringList &options);

  void addCustomSetting(const QString &settingKey, QtProperty *settingProperty);

  void addOptionalDoubleSetting(const QString &settingKey,
                                const QString &settingName,
                                const QString &optionKey,
                                const QString &optionName, bool enabled = false,
                                double defaultValue = 0);

  void addOptionalSetting(const QString &settingKey,
                          QtProperty *settingProperty, const QString &optionKey,
                          const QString &optionName, bool enabled = false);

  void updateParameterValues(const QHash<QString, double> &parameterValues);

  void updateParameterValues(PropertyHandler *functionHandler,
                             const QHash<QString, double> &parameterValues);

  void removeFunction(PropertyHandler *handler) override;

public slots:
  void fit() override;

  void sequentialFit() override;

protected slots:
  void enumChanged(QtProperty *prop) override;

  void boolChanged(QtProperty *prop) override;

  void intChanged(QtProperty *prop) override;

  void clearCustomFunctions();

signals:
  void customBoolChanged(const QString &settingName, bool value);

  void customIntChanged(const QString &settingName, int value);

  void customEnumChanged(const QString &settingName, const QString &value);

  void fitScheduled();

  void sequentialFitScheduled();

private:
  void addCustomFunctionGroup(
      const QString &groupName,
      const std::vector<Mantid::API::IFunction_sptr> &functionNames);

  void addCustomFunctions(QtProperty *prop, const QString &groupName);

  void addCustomFunctions(QtProperty *prop, const QString &groupName,
                          const int &multiples);

  void clearCustomFunctions(QtProperty *prop);

  QtProperty *
  createFunctionGroupProperty(const QString &groupName,
                              QtAbstractPropertyManager *propertyManager,
                              bool atFront = false);

  QString enumValue(QtProperty *prop) const;

  QtProperty *m_customFunctionGroups;
  QtProperty *m_backgroundGroup;
  QtProperty *m_customSettingsGroup;
  QtProperty *m_functionsInComboBox;
  QSet<QtProperty *> m_functionsAsCheckBox;
  QSet<QtProperty *> m_functionsAsSpinner;
  QHash<QString, QtProperty *> m_customSettings;
  QtProperty *m_backgroundSelection;
  PropertyHandler *m_backgroundHandler;
  QHash<QtProperty *, QVector<PropertyHandler *>> m_functionHandlers;
  QVector<QtProperty *> m_orderedFunctionGroups;
  std::unordered_map<std::string, size_t> m_customFunctionCount;
  QSet<QtProperty *> m_optionProperties;
  QHash<QtProperty *, QtProperty *> m_optionalProperties;

  std::string selectedBackground;
  QHash<QString, std::vector<Mantid::API::IFunction_sptr>>
      m_groupToFunctionList;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*INDIRECTFITPROPERTYBROWSER_H_*/
