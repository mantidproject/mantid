// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/QtPropertyBrowser/StringDialogEditor.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * A stringDialogEditor for editing file names.
 */
class FilenameDialogEditor : public StringDialogEditor {
  Q_OBJECT
public:
  FilenameDialogEditor(QtProperty *property, QWidget *parent) : StringDialogEditor(property, parent) {}
protected slots:
  void runDialog() override;
};

/**
 * The factory for the FilenameDialogEditor.
 */
class FilenameDialogEditorFactory : public StringDialogEditorFactory {
  Q_OBJECT
public:
  FilenameDialogEditorFactory(QObject *parent) : StringDialogEditorFactory(parent) {}

protected:
  using QtAbstractEditorFactoryBase::createEditor; // Avoid Intel compiler
                                                   // warning
  QWidget *createEditorForManager(QtStringPropertyManager *manager, QtProperty *property, QWidget *parent) override {
    (void)manager; // Avoid unused warning
    return new FilenameDialogEditor(property, parent);
  }
};
} // namespace MantidWidgets
} // namespace MantidQt
