// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_FORMULADIALOGEDIT_H
#define MANTIDQT_MANTIDWIDGETS_FORMULADIALOGEDIT_H

#include "StringDialogEditor.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * A stringDialogEditor for editing UserFunction.
 */
class FormulaDialogEditor : public StringDialogEditor {
  Q_OBJECT
public:
  FormulaDialogEditor(QtProperty *property, QWidget *parent)
      : StringDialogEditor(property, parent) {}
protected slots:
  void runDialog() override;
};

/**
 * The factory for the FormulaDialogEditor.
 */
class FormulaDialogEditorFactory : public StringDialogEditorFactory {
  Q_OBJECT
public:
  FormulaDialogEditorFactory(QObject *parent)
      : StringDialogEditorFactory(parent) {}

protected:
  QWidget *createEditorForManager(QtStringPropertyManager * /*manager*/,
                                  QtProperty *property,
                                  QWidget *parent) override {
    return new FormulaDialogEditor(property, parent);
  }
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_MANTIDWIDGETS_FORMULADIALOGEDIT_H
