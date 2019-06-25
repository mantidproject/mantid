// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef STRINGEDITORFACTORY_H
#define STRINGEDITORFACTORY_H

#include "qtpropertymanager.h"
#include <QLineEdit>

class StringEditorFactory
    : public QtAbstractEditorFactory<QtStringPropertyManager> {
  Q_OBJECT
public:
  StringEditorFactory(QObject *parent = nullptr)
      : QtAbstractEditorFactory<QtStringPropertyManager>(parent) {}

protected:
  using QtAbstractEditorFactoryBase::createEditor; // Avoid Intel compiler
                                                   // warning
  void connectPropertyManager(QtStringPropertyManager * /*manager*/) override {}
  QWidget *createEditorForManager(QtStringPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void
  disconnectPropertyManager(QtStringPropertyManager * /*manager*/) override {}
};

class StringEditor : public QLineEdit {
  Q_OBJECT
public:
  StringEditor(QtProperty *property, QWidget *parent);
protected slots:
  void updateProperty();

private:
  QtProperty *m_property;
};

#endif // STRINGEDITORFACTORY_H
