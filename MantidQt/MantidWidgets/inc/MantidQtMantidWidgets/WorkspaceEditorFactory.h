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
  using QtAbstractEditorFactoryBase::createEditor; // Avoid Intel compiler warning
  void connectPropertyManager(QtStringPropertyManager *){}
  QWidget *createEditorForManager(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent);
  void disconnectPropertyManager(QtStringPropertyManager *){}
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
