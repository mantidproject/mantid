// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DOUBLEDIALOGEDITORFACTORY_H
#define DOUBLEDIALOGEDITORFACTORY_H

#include "DoubleEditorFactory.h"
#include "ParameterPropertyManager.h"

class QCheckBox;
class QLineEdit;
class QPushButton;

/**
 * Partially implemented string editor. It has a QLineEdit for manual editing
 * and a button [...] next to it
 * to call a dialog for more complex editing. Clicking the button calls virtual
 * runDialog() method.
 * Concrete classes must implement it.
 */
class DoubleDialogEditor : public QWidget {
  Q_OBJECT
public:
  DoubleDialogEditor(QtProperty *property, QWidget *parent, bool hasOption = false, bool isOptionSet = false);
signals:
  void buttonClicked(QtProperty * /*_t1*/);
  void closeEditor();

protected slots:
  /// Implementations must open a dialog to edit the editor's text. If editing
  /// is successful
  /// setText() and updateProperty() methods must be called.
  void runDialog();
  void optionToggled(bool);
  void updateProperty();
  void setText(const QString &txt);
  QString getText() const;

private:
  bool eventFilter(QObject * /*obj*/, QEvent * /*evt*/) override;
  DoubleEditor *m_editor;
  QPushButton *m_button;
  QCheckBox *m_checkBox;
  QtProperty *m_property;
  bool m_hasOption;
  bool m_isOptionSet;
};

/**
 * An abstract editor factory to be used with QtPropertyBrowser. Implementations
 * need to
 * implement QWidget *createEditor(QtStringPropertyManager *manager, QtProperty
 * *property,QWidget *parent)
 * method which creates a specific editor. The underlying type of the edited
 * property must be string.
 */
class EXPORT_OPT_MANTIDQT_COMMON DoubleDialogEditorFactory
    : public QtAbstractEditorFactory<ParameterPropertyManager> {
  Q_OBJECT
public:
  DoubleDialogEditorFactory(QObject *parent = nullptr, bool hasOption = false)
      : QtAbstractEditorFactory<ParameterPropertyManager>(parent), m_hasOption(hasOption) {}
  QWidget *createEditorForManager(ParameterPropertyManager *,
    QtProperty *property,
    QWidget *parent) override;
signals:
  void buttonClicked(QtProperty * /*_t1*/);
  void closeEditor();

protected:
  void connectPropertyManager(ParameterPropertyManager * /*manager*/) override {
  }
  void
  disconnectPropertyManager(ParameterPropertyManager * /*manager*/) override {}
  bool m_hasOption;
};

#endif // DOUBLEDIALOGEDITORFACTORY_H
