#ifndef MANTIDQT_MANTIDWIDGETS_FILENAMEDIALOGEDITFACTORY_H
#define MANTIDQT_MANTIDWIDGETS_FILENAMEDIALOGEDITFACTORY_H

#include "MantidQtMantidWidgets/StringDialogEditor.h"

namespace MantidQt
{
namespace MantidWidgets
{

/**
 * A stringDialogEditor for editing file names.
 */
class FilenameDialogEditor: public StringDialogEditor
{
  Q_OBJECT
public:
  FilenameDialogEditor(QtProperty *property, QWidget *parent)
    :StringDialogEditor(property,parent){}
protected slots:
  void runDialog();
};

/**
 * The factory for the FilenameDialogEditor.
 */
class FilenameDialogEditorFactory: public StringDialogEditorFactory
{
  Q_OBJECT
public:
  FilenameDialogEditorFactory(QObject* parent):StringDialogEditorFactory(parent){}
protected:
  using QtAbstractEditorFactoryBase::createEditor; // Avoid Intel compiler warning
  QWidget *createEditorForManager(QtStringPropertyManager *manager, QtProperty *property,QWidget *parent)
  {
    (void) manager; //Avoid unused warning
    return new FilenameDialogEditor(property,parent);
  }
};

}
}

#endif // MANTIDQT_MANTIDWIDGETS_FILENAMEDIALOGEDITFACTORY_H