#ifndef MANTIDQT_MANAGE_USER_DIRECTORIES_H
#define MANTIDQT_MANAGE_USER_DIRECTORIES_H

#include <QDialog>
#include "MantidQtAPI/ui_ManageUserDirectories.h"
#include "DllOption.h"

namespace MantidQt
{
namespace API
{

class EXPORT_OPT_MANTIDQT_API ManageUserDirectories : public QDialog
{
	Q_OBJECT
public:
	ManageUserDirectories(QWidget *parent = 0);
	~ManageUserDirectories();

private:
  virtual void initLayout();
  void loadProperties();
  void saveProperties();

private slots:
  void helpClicked();
  void cancelClicked();
  void confirmClicked();

  void addDataDir();
  void remDataDir();
  void moveUp();
  void moveDown();
  void selectSaveDir();

private:
	Ui::ManageUserDirectories m_uiForm;
  QString m_userPropFile;
	QString m_saveDir;
	QStringList m_dataDirs;


};

} // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_MANAGE_USER_DIRECTORIES_H */