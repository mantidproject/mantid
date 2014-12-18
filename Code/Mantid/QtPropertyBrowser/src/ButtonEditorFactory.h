#ifndef BUTTONEDITORFACTORY_H
#define BUTTONEDITORFACTORY_H

#include "qtpropertymanager.h"
#include <QPushButton>

class QT_QTPROPERTYBROWSER_EXPORT ButtonEditor: public QPushButton
{
  Q_OBJECT
public:
  ButtonEditor(QtProperty *property, QWidget *parent):
    QPushButton("...",parent), m_property(property)
  {
    connect(this,SIGNAL(clicked()),this,SLOT(sendClickedSignal()));
  }
Q_SIGNALS:
  void buttonClicked(QtProperty *);
private Q_SLOTS:
  void sendClickedSignal()
  {
    emit buttonClicked(m_property);
  }

private:
  QtProperty* m_property;
};

template <class ManagerType>
class ButtonEditorFactory : public QtAbstractEditorFactory<ManagerType>
{
//  Q_OBJECT
public:
  ButtonEditorFactory(QObject *parent)
    : QtAbstractEditorFactory<ManagerType>(parent)
  {
  }

protected:
  void connectPropertyManager(ManagerType *manager)
  {
    (void) manager; // Unused
    // Do nothing
  }

  void disconnectPropertyManager(ManagerType *manager)
  {
    (void) manager; // Unused
    // Do nothing
  }

  QWidget *createEditor(ManagerType *manager, QtProperty *property, QWidget *parent)
  {
    (void) manager; // Unused
    auto button = new ButtonEditor(property, parent);
    this->connect(button,SIGNAL(buttonClicked(QtProperty *)),this,SIGNAL(buttonClicked(QtProperty *)));
    return button;
  }
};

class QT_QTPROPERTYBROWSER_EXPORT DoubleButtonEditorFactory: public ButtonEditorFactory<QtDoublePropertyManager>
{
  Q_OBJECT

public:
  DoubleButtonEditorFactory(QObject *parent):ButtonEditorFactory<QtDoublePropertyManager>(parent){}

Q_SIGNALS:
  void buttonClicked(QtProperty *);

};


#endif // BUTTONEDITORFACTORY_H
