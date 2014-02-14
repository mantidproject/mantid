#ifndef STRINGEDITORFACTORY_H
#define STRINGEDITORFACTORY_H

#include "qtpropertymanager.h"
#include "MantidQtMantidWidgets/WorkspaceSelector.h"

namespace MantidQt
{
namespace MantidWidgets
{

class WorkspaceEditorFactory : public QtAbstractEditorFactory<QtStringPropertyManager>
{
    Q_OBJECT
public:
  WorkspaceEditorFactory(QObject *parent = 0): QtAbstractEditorFactory<QtStringPropertyManager>(parent){}
protected:
  void connectPropertyManager(QtStringPropertyManager *manager){}
  QWidget *createEditor(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent);
  void disconnectPropertyManager(QtStringPropertyManager *manager){}
};

class WorkspaceEditor: public WorkspaceSelector
{
  Q_OBJECT
public:
  WorkspaceEditor(QtProperty *property, QWidget *parent);
protected slots:
  void updateProperty(const QString& text);
private:
  QtProperty* m_property;
};

}
}

#endif // STRINGEDITORFACTORY_H
