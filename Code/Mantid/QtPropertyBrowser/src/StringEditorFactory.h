#ifndef STRINGEDITORFACTORY_H
#define STRINGEDITORFACTORY_H

#include "qtpropertymanager.h"
#include <QLineEdit>

class QT_QTPROPERTYBROWSER_EXPORT StringEditorFactory : public QtAbstractEditorFactory<QtStringPropertyManager>
{
    Q_OBJECT
public:
  StringEditorFactory(QObject *parent = 0): QtAbstractEditorFactory<QtStringPropertyManager>(parent){}
protected:
  void connectPropertyManager(QtStringPropertyManager *manager);
  QWidget *createEditor(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent);
  void disconnectPropertyManager(QtStringPropertyManager *manager);
};

class QT_QTPROPERTYBROWSER_EXPORT StringEditor: public QLineEdit
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
