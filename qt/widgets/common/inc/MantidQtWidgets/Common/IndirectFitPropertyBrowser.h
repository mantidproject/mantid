// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INDIRECTFITPROPERTYBROWSER_H_
#define INDIRECTFITPROPERTYBROWSER_H_

#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"

#include <QSet>

#include <boost/optional.hpp>

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

  boost::optional<size_t> backgroundIndex() const;

  boost::optional<size_t>
  functionIndex(Mantid::API::IFunction_sptr function) const;

  QString selectedFitType() const;

  QString backgroundName() const;

  QHash<QString, QString> getTies() const;

  void getCompositeTies(PropertyHandler *handler,
                        QHash<QString, QString> &ties) const;

  void getTies(PropertyHandler *handler, QHash<QString, QString> &ties) const;

  size_t numberOfCustomFunctions(const std::string &functionName) const;

  std::vector<double> parameterValue(const std::string &functionName,
                                     const std::string &parameterName) const;

  void setParameterValue(const std::string &functionName,
                         const std::string &parameterName, double value);

  void setParameterValue(Mantid::API::IFunction_sptr function,
                         const std::string &parameterName, double value);

  void setBackground(const std::string &backgroundName);

  void setConvolveMembers(bool convolveMembers);

  void setCustomSettingEnabled(const QString &customName, bool enabled);

  void setFitEnabled(bool enable) override;

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
  void clearFitTypeComboBox();

  void setBackgroundOptions(const QStringList &backgrounds);

  bool boolSettingValue(const QString &settingKey) const;

  void setCustomBoolSetting(const QString &settingKey, bool value);

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

  void setCustomSettingChangesFunction(const QString &settingKey,
                                       bool changesFunction);

  void updateTies();
  void updateTie(std::size_t index);
  void addTie(const QString &tie);
  void removeTie(const QString &parameter);
  void updateErrors();
  void clearErrors();

  void removeFunction(PropertyHandler *handler) override;

  void setWorkspaceIndex(int i) override;

  void updatePlotGuess(Mantid::API::MatrixWorkspace_const_sptr sampleWorkspace);

public slots:
  void fit() override;
  void sequentialFit() override;

protected slots:
  void enumChanged(QtProperty *prop) override;

  void boolChanged(QtProperty *prop) override;

  void intChanged(QtProperty *prop) override;

  void doubleChanged(QtProperty *prop) override;

  void customChanged(QtProperty *settingName);

  void clear() override;

  void clearAllCustomFunctions();

  void browserVisibilityChanged(bool isVisible);

signals:
  void customBoolChanged(const QString &settingName, bool value);

  void customIntChanged(const QString &settingName, int value);

  void customEnumChanged(const QString &settingName, const QString &value);

  void customDoubleChanged(const QString &settingName, double value);

  void customSettingChanged(QtProperty *settingName);

  void fitScheduled();

  void sequentialFitScheduled();

  void browserClosed();

protected:
  void addWorkspaceIndexToBrowser() override {}

private:
  void addCustomFunctionGroup(
      const QString &groupName,
      const std::vector<Mantid::API::IFunction_sptr> &functions);

  void addCustomFunctions(QtProperty *prop, const QString &groupName);
  void addCustomFunctions(QtProperty *prop, const QString &groupName,
                          const int &multiples);
  void
  addCustomFunctions(QtProperty *prop,
                     const std::vector<Mantid::API::IFunction_sptr> &functions);

  void clearCustomFunctions(QtProperty *prop, bool emitSignals);
  void clearCustomFunctions(QtProperty *prop);

  void customFunctionRemoved(QtProperty *property);

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
  QSet<QtProperty *> m_functionChangingSettings;
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
