#ifndef MDFADDWORKSPACEDIALOG_H_
#define MDFADDWORKSPACEDIALOG_H_

#include <QDialog>
#include "ui_MDFAddWorkspaceDialog.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{

/**
  * A dialog for selecting a workspace from the ADS.
  */
class AddWorkspaceDialog: public QDialog
{
  Q_OBJECT
public:
  AddWorkspaceDialog(QWidget *parent);
  QString workspaceName() const {return m_workspaceName;} 
  std::vector<int> workspaceIndices() const {return m_wsIndices;}
private slots:
  void accept();
  void reject();
  void workspaceNameChanged(const QString&);
  void selectAllSpectra(int state);
private:
  /// Name of the selected workspace
  QString m_workspaceName;
  /// Selected workspace index
  std::vector<int> m_wsIndices;
  /// Maximum index in the selected workspace
  int m_maxIndex;
  Ui::MDFAddWorkspaceDialog m_uiForm;
};

} // MDF
} // CustomInterfaces
} // MantidQt


#endif /*MDFADDWORKSPACEDIALOG_H_*/
