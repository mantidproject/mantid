#ifndef MANTIDQT_MANTIDWIDGETS_FILENAMEDIALOGEDITFACTORY_H
#define MANTIDQT_MANTIDWIDGETS_FILENAMEDIALOGEDITFACTORY_H

#include "StringDialogEditorFactory.h"

namespace MantidQt
{
namespace MantidWidgets
{

class QT_QTPROPERTYBROWSER_EXPORT FilenameDialogEditor: public StringDialogEditor
{
  Q_OBJECT
public:
  FilenameDialogEditor(QtProperty *property, QWidget *parent)
    :StringDialogEditor(property,parent){}
protected slots:
  void runDialog();
};


class QT_QTPROPERTYBROWSER_EXPORT FilenameDialogEditorFactory: public StringDialogEditorFactory
{
  Q_OBJECT
public:
  FilenameDialogEditorFactory(QObject* parent):StringDialogEditorFactory(parent){}
protected:
  using QtAbstractEditorFactoryBase::createEditor; // Avoid Intel compiler warning
  QWidget *createEditor(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent)
  {
    (void) manager; //Avoid unused warning
    return new FilenameDialogEditor(property,parent);
  }
};

}
}

#endif // MANTIDQT_MANTIDWIDGETS_FILENAMEDIALOGEDITFACTORY_H