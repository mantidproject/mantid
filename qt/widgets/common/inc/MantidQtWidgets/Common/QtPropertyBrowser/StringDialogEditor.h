// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "qtpropertymanager.h"

class QLineEdit;

/**
 * An abstract editor factory to be used with QtPropertyBrowser. Implementations
 * need to
 * implement QWidget *createEditor(QtStringPropertyManager *manager, QtProperty
 * *property,QWidget *parent)
 * method which creates a specific editor. The underlying type of the edited
 * property must be string.
 */
class StringDialogEditorFactory : public QtAbstractEditorFactory<QtStringPropertyManager> {
  Q_OBJECT
public:
  StringDialogEditorFactory(QObject *parent = nullptr) : QtAbstractEditorFactory<QtStringPropertyManager>(parent) {}

protected:
  void connectPropertyManager(QtStringPropertyManager *manager) override;
  void disconnectPropertyManager(QtStringPropertyManager *manager) override;
};

/**
 * Partially implemented string editor. It has a QLineEdit for manual editing
 * and a button [...] next to it
 * to call a dialog for more complex editing. Clicking the button calls virtual
 * runDialog() method.
 * Concrete classes must implement it.
 */
class StringDialogEditor : public QWidget {
  Q_OBJECT
public:
  StringDialogEditor(QtProperty *property, QWidget *parent);
  ~StringDialogEditor() override;
protected slots:
  /// Implementations must open a dialog to edit the editor's text. If editing
  /// is successful
  /// setText() and updateProperty() methods must be called.
  virtual void runDialog() = 0;
  void updateProperty();
  void setText(const QString &txt);
  QString getText() const;

private:
  QLineEdit *m_lineEdit;
  QtProperty *m_property;
};
