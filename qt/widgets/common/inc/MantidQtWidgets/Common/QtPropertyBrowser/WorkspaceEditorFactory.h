#ifndef STRINGEDITORFACTORY_H
#define STRINGEDITORFACTORY_H

#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/WorkspaceSelector.h"

namespace MantidQt {
namespace MantidWidgets {

class WorkspaceEditorFactory
    : public QtAbstractEditorFactory<QtStringPropertyManager> {
  Q_OBJECT
public:
  WorkspaceEditorFactory(QObject *parent = nullptr)
      : QtAbstractEditorFactory<QtStringPropertyManager>(parent) {}

protected:
  using QtAbstractEditorFactoryBase::createEditor; // Avoid Intel compiler
                                                   // warning
  void connectPropertyManager(QtStringPropertyManager *) override {}
  QWidget *createEditorForManager(QtStringPropertyManager *manager,
                                  QtProperty *property,
                                  QWidget *parent) override;
  void disconnectPropertyManager(QtStringPropertyManager *) override {}
};

class WorkspaceEditor : public WorkspaceSelector {
  Q_OBJECT
public:
  WorkspaceEditor(QtProperty *property, QWidget *parent);
protected slots:
  void updateProperty(const QString &text);

private:
  QtProperty *m_property;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // STRINGEDITORFACTORY_H
