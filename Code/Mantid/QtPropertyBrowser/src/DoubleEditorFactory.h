#ifndef DOUBLEEDITORFACTORY_H
#define DOUBLEEDITORFACTORY_H

#include "qtpropertymanager.h"

#include <QLineEdit>

class QT_QTPROPERTYBROWSER_EXPORT DoubleEditorFactory : public QtAbstractEditorFactory<QtDoublePropertyManager>
{
    Q_OBJECT
public:
  DoubleEditorFactory(QObject *parent = 0): QtAbstractEditorFactory<QtDoublePropertyManager>(parent){}
protected:
  void connectPropertyManager(QtDoublePropertyManager *manager);
    QWidget *createEditor(QtDoublePropertyManager *manager, QtProperty *property,
      QWidget *parent);
    void disconnectPropertyManager(QtDoublePropertyManager *manager);
};

class QT_QTPROPERTYBROWSER_EXPORT DoubleEditor: public QLineEdit
{
  Q_OBJECT
public:
  DoubleEditor(QtProperty *property, QWidget *parent);
  ~DoubleEditor();
  void setValue(const double& d);
private slots:
  void updateProperty();
private:
  QtProperty* m_property;
  int m_decimals;
};

#endif // DOUBLEEDITORFACTORY_H
