// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "StringDialogEditor.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * A stringDialogEditor for editing UserFunction.
 */
class FormulaDialogEditor : public StringDialogEditor {
  Q_OBJECT
public:
  FormulaDialogEditor(QtProperty *property, QWidget *parent) : StringDialogEditor(property, parent) {}
protected slots:
  void runDialog() override;
};

/**
 * The factory for the FormulaDialogEditor.
 */
class FormulaDialogEditorFactory : public StringDialogEditorFactory {
  Q_OBJECT
public:
  FormulaDialogEditorFactory(QObject *parent) : StringDialogEditorFactory(parent) {}

protected:
  QWidget *createEditorForManager(QtStringPropertyManager * /*manager*/, QtProperty *property,
                                  QWidget *parent) override {
    return new FormulaDialogEditor(property, parent);
  }
};
} // namespace MantidWidgets
} // namespace MantidQt
