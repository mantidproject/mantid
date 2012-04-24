// Implementation for the QtMwsRemoteJobManager class (which really means just the getPassword() function,
// since everything else is handled by the MwsRemoteJobManager class).
// This is a separate class just to keep the Qt stuff isolated.


#include "RemoteJobManager.h"

#include <QString>
#include <QInputDialog>
#include <QUrl>

bool QtMwsRemoteJobManager::getPassword()
{
  bool isOk;


  QString msg = QObject::tr("Enter password for ") + QString::fromStdString(m_userName) + QString("@") + QUrl(QString::fromStdString(m_serviceBaseUrl)).encodedHost();
  QString text = QInputDialog::getText( NULL,
                                        QInputDialog::tr("Password"),
                                        msg,
                                        QLineEdit::Password,
                                        QString::fromStdString(m_password),
                                        &isOk);

  if (isOk)
  {
    MwsRemoteJobManager::m_password = text.toStdString();
  }
  else
  {
    m_password.clear();
  }

  return isOk;
}
