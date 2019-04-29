// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef BUTTONEDITORFACTORY_H
#define BUTTONEDITORFACTORY_H

#include "ParameterPropertyManager.h"
#include "qtpropertymanager.h"
#include <QPushButton>

class EXPORT_OPT_MANTIDQT_COMMON ButtonEditor : public QPushButton {
  Q_OBJECT
public:
  ButtonEditor(QtProperty *property, QWidget *parent)
      : QPushButton("...", parent), m_property(property) {
    connect(this, SIGNAL(clicked()), this, SLOT(sendClickedSignal()));
  }
Q_SIGNALS:
  void buttonClicked(QtProperty * /*_t1*/);
private Q_SLOTS:
  void sendClickedSignal() { emit buttonClicked(m_property); }

private:
  QtProperty *m_property;
};

template <class ManagerType>
class ButtonEditorFactory : public QtAbstractEditorFactory<ManagerType> {
  //  Q_OBJECT
public:
  ButtonEditorFactory(QObject *parent)
      : QtAbstractEditorFactory<ManagerType>(parent) {}

protected:
  void connectPropertyManager(ManagerType *manager) override {
    (void)manager; // Unused
    // Do nothing
  }

  void disconnectPropertyManager(ManagerType *manager) override {
    (void)manager; // Unused
    // Do nothing
  }

  QWidget *createEditorForManager(ManagerType *manager, QtProperty *property,
                                  QWidget *parent) override {
    (void)manager; // Unused
    auto button = new ButtonEditor(property, parent);
    this->connect(button, SIGNAL(buttonClicked(QtProperty *)), this,
                  SIGNAL(buttonClicked(QtProperty *)));
    return button;
  }
};

class EXPORT_OPT_MANTIDQT_COMMON DoubleButtonEditorFactory
    : public ButtonEditorFactory<ParameterPropertyManager> {
  Q_OBJECT

public:
  DoubleButtonEditorFactory(QObject *parent)
      : ButtonEditorFactory<ParameterPropertyManager>(parent) {}

Q_SIGNALS:
  void buttonClicked(QtProperty * /*_t1*/);
};

#endif // BUTTONEDITORFACTORY_H
