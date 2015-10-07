#ifndef MANTIDQTMANTIDWIDGETS_RENAMEPARDIALOG_H_
#define MANTIDQTMANTIDWIDGETS_RENAMEPARDIALOG_H_

#include "ui_RenameParDialog.h"
#include "WidgetDllOption.h"

#include <vector>
#include <string>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
     * A dialog for renaming parameters for a user function
     */
	  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS RenameParDialog : public QDialog
    {
      Q_OBJECT

    public:
      /// there has to be a default constructor but you can call it with a pointer to the thing that will take ownership of it
      RenameParDialog(const std::vector<std::string>& old_params,
        const std::vector<std::string>& new_params,
        QWidget *parent=NULL);
      void setOutput(std::vector<std::string>& out)const;
    protected slots:
      void uniqueIndexedNames(bool);
      void doNotRename(bool);
    protected:
      bool isUnique(const QString& name)const;
      QString makeUniqueIndexedName(const QString& name);
      /// User interface elements
      Ui::RenameParDialog m_uiForm;
      const std::vector<std::string> m_old_params;
      const std::vector<std::string> m_new_params;
	  };
  }
}

#endif //MANTIDQTMANTIDWIDGETS_RENAMEPARDIALOG_H_
