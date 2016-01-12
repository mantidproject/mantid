#ifndef STRINGEDITORFACTORY_H
#define STRINGEDITORFACTORY_H

#include "qtpropertymanager.h"
#include <QLineEdit>

class StringEditorFactory : public QtAbstractEditorFactory<QtStringPropertyManager>
{
    Q_OBJECT
public:
  StringEditorFactory(QObject *parent = 0): QtAbstractEditorFactory<QtStringPropertyManager>(parent){}
protected:
  using QtAbstractEditorFactoryBase::createEditor; // Avoid Intel compiler warning
  void connectPropertyManager(QtStringPropertyManager *){}
  QWidget *createEditorForManager(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent);
  void disconnectPropertyManager(QtStringPropertyManager *){}
};

class StringEditor: public QLineEdit
{
  Q_OBJECT
public:
  StringEditor(QtProperty *property, QWidget *parent);
protected slots:
  void updateProperty();
private:
  QtProperty* m_property;
};

#endif // STRINGEDITORFACTORY_H
