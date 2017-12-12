#ifndef INDIRECTFITPROPERTYBROWSER_H_
#define INDIRECTFITPROPERTYBROWSER_H_

#include "MantidQtWidgets/Common/FitPropertyBrowser.h"

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON IndirectFitPropertyBrowser
    : public FitPropertyBrowser {
  Q_OBJECT

public:
  enum class CustomGroupMode { COMBOBOX, CHECKBOX };

  /// Constructor.
  IndirectFitPropertyBrowser(QWidget *parent = nullptr,
                             QObject *mantidui = nullptr);
  /// Initialise the layout.
  void init() override;

  void addCustomFunctionGroup(const QString &groupName,
                              std::vector<std::string> functionNames,
                              CustomGroupMode mode = CustomGroupMode::COMBOBOX);

  void updateParameterValues(QHash<QString, double> parameterValues);

protected slots:
  void enumChanged(QtProperty *prop) override;

  void boolChanged(QtProperty *prop) override;

private:
  void addCustomFunctionGroupToComboBox(const QString &groupName);

  void addCustomFunctions(QtProperty *prop, const QString &groupName);

  void clearCustomFunctions(QtProperty *prop);

  QString enumValue(QtProperty *prop) const;

  QtProperty *m_customFunctionGroups;
  QtProperty *m_functionsInComboBox;
  QSet<QtProperty *> m_functionsAsCheckBox;
  QtProperty *m_backgroundSelection;
  PropertyHandler *m_backgroundHandler;
  QHash<QtProperty *, QVector<PropertyHandler *>> m_functionHandlers;

  std::string selectedBackground;
  QHash<QString, std::vector<std::string>> m_groupToFunctionList;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*INDIRECTFITPROPERTYBROWSER_H_*/
