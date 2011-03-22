#ifndef STRINGDIALOGEDITORFACTORY_H
#define STRINGDIALOGEDITORFACTORY_H

#include "qtpropertymanager.h"

class QLineEdit;

class QT_QTPROPERTYBROWSER_EXPORT StringDialogEditorFactory : public QtAbstractEditorFactory<QtStringPropertyManager>
{
    Q_OBJECT
public:
  StringDialogEditorFactory(QObject *parent = 0): QtAbstractEditorFactory<QtStringPropertyManager>(parent){}
protected:
  void connectPropertyManager(QtStringPropertyManager *manager);
  QWidget *createEditor(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent);
  void disconnectPropertyManager(QtStringPropertyManager *manager);
};

class QT_QTPROPERTYBROWSER_EXPORT StringDialogEditor: public QWidget
{
  Q_OBJECT
public:
  StringDialogEditor(QtProperty *property, QWidget *parent);
  ~StringDialogEditor();
protected slots:
  virtual void runDialog();
  void updateProperty();
  void setText(const QString& txt);
  QString getText()const;
private:
  QLineEdit* m_lineEdit;
  QtProperty* m_property;
};

#endif // STRINGDIALOGEDITORFACTORY_H
