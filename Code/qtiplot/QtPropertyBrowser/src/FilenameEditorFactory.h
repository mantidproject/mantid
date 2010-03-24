#ifndef FILENAMEEDITORFACTORY_H
#define FILENAMEEDITORFACTORY_H

#include "qtpropertymanager.h"

class QLineEdit;

class QT_QTPROPERTYBROWSER_EXPORT FilenameEditorFactory : public QtAbstractEditorFactory<QtStringPropertyManager>
{
    Q_OBJECT
public:
  FilenameEditorFactory(QObject *parent = 0): QtAbstractEditorFactory<QtStringPropertyManager>(parent){}
protected:
  void connectPropertyManager(QtStringPropertyManager *manager);
    QWidget *createEditor(QtStringPropertyManager *manager, QtProperty *property,
      QWidget *parent);
    void disconnectPropertyManager(QtStringPropertyManager *manager);
};

class QT_QTPROPERTYBROWSER_EXPORT FilenameEditor: public QWidget
{
  Q_OBJECT
public:
  FilenameEditor(QtProperty *property, QWidget *parent);
  ~FilenameEditor();
private slots:
  void runDialog();
  void updateProperty();
private:
  QLineEdit* m_lineEdit;
  QtProperty* m_property;
};

#endif
