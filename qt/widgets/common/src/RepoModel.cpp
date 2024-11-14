// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/RepoModel.h"

#include "MantidAPI/ScriptRepositoryFactory.h"
#include <utility>
#include <vector>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidQtIcons/Icon.h"
#include <QIcon>
#include <QPixmap>

#include <QCheckBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtConcurrentRun>
#include <stdexcept>

using namespace MantidQt::API;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::ConfigServiceImpl;

namespace {
/// static logger
Mantid::Kernel::Logger g_log("RepoModel");
} // namespace

// flag to indicate that the thread is delete thread
const char *delete_mark = "*DELETE*";
const char *nofile_flag = "nofile";

/// Executes the download from ScriptRepository. This function will be executed
/// in a separate thread
static QString download_thread(Mantid::API::ScriptRepository_sptr &pt, const std::string &path) {
  QString result;
  try {
    pt->download(path);
  } catch (Mantid::API::ScriptRepoException &ex) {
    QString info = QString::fromStdString(ex.what());
    // make the exception a nice html message
    info.replace("\n", "</p><p>");
    return info;
  }
  return result;
}

/// Execute the upload from ScriptRepository. This function will be executed in
/// a separate thread
static QString upload_thread(Mantid::API::ScriptRepository_sptr &pt, const std::string &path, const QString &email,
                             const QString &author, const QString &comment) {
  try {
    pt->upload(path, comment.toStdString(), author.toStdString(), email.toStdString());
  } catch (Mantid::API::ScriptRepoException &ex) {
    QString info = QString::fromStdString(ex.what());
    info.replace("\n", "</p><p>");
    return info;
  }
  return QString();
}
/// Execute the remove from ScriptRepository. This function will be executed in
/// a separate thread.
static QString delete_thread(Mantid::API::ScriptRepository_sptr &pt, const std::string &path, const QString &email,
                             const QString &author, const QString &comment) {
  try {
    pt->remove(path, comment.toStdString(), author.toStdString(), email.toStdString());
  } catch (Mantid::API::ScriptRepoException &ex) {
    QString info = QString::fromStdString(ex.what());
    info.replace("\n", "</p><p>");
    // it adds the mark *DELETE* so to recognize that it was used to delete an
    // entry.
    return info + delete_mark;
  }
  return delete_mark;
}

/*
  An auxiliary nested class to help RepoModel to rebuild the hierarchical
  tree of ScriptRepository.

  It will keep track of the path that is the main key to access metadata
  on ScriptRepository and an auxiliary label to easy the display on a
  user nicer way.

*/
RepoModel::RepoItem::RepoItem(QString label, QString path, RepoItem *parent)
    : m_label(std::move(label)), keypath(std::move(path)), parentItem(parent) {}
/// destruct all the childItems.
RepoModel::RepoItem::~RepoItem() { qDeleteAll(childItems); }
/** This method is the very responsible to allow the reconstruction of the
    hierarchical tree, keeping track of the children of one item.
 */
void RepoModel::RepoItem::appendChild(RepoItem *child) { childItems.append(child); }

/** Gives access to the row_th children of RepoItem. Note that
    the row can not be greater than RepoModel::childCount().

    But we do not test it, because this method will be called
    indirectly from the QView, and will never go beyond that value.

    @param row: Number between 0 and RepoModel::childCount().

    @return The pointer to the row_th children.

 */
RepoModel::RepoItem *RepoModel::RepoItem::child(int row) const { return childItems.value(row); }

/** Return the number of children that this entry may find.
    @return Number of children
 */
int RepoModel::RepoItem::childCount() const { return childItems.count(); }
/** Provide the row number of this entry related to its parent.
    @return It's child position.
 */
int RepoModel::RepoItem::row() const {
  if (parentItem)
    return parentItem->childItems.indexOf(const_cast<RepoItem *>(this));
  return 0;
}
/** Remove the given child from the childItems. Used to allow removing rows from
 * the view*/
bool RepoModel::RepoItem::removeChild(int row) {
  if (row < 0 || row >= childCount())
    return false;

  childItems.removeAt(row);
  return true;
}

//////////////////////////////////////////////////
// MODEL
///////////////////////////////////////////////////
/** The constructor of RepoModel. It is intended to have just one model and to
   pass
    on through shared pointer. To enforce this, the copy constructor is
   disabled.
    It will construct the hierarchical tree of ScriptRepository through
    the setuptModelData.
 */
RepoModel::RepoModel(QObject *parent) : QAbstractItemModel(parent) {
  const ConfigServiceImpl &config = ConfigService::Instance();
  repo_path = QString::fromStdString(config.getString("ScriptLocalRepository"));
  rootItem = new RepoItem("/");
  using Mantid::API::ScriptRepository;
  using Mantid::API::ScriptRepository_sptr;
  using Mantid::API::ScriptRepositoryFactory;
  repo_ptr = ScriptRepositoryFactory::Instance().create("ScriptRepositoryImpl");
  connect(&download_watcher, SIGNAL(finished()), this, SLOT(downloadFinished()));
  connect(&upload_watcher, SIGNAL(finished()), this, SLOT(uploadFinished()));
  uploading_path = nofile_flag;
  downloading_path = nofile_flag;
  setupModelData(rootItem);
}

/// desctructor of repomodel.
RepoModel::~RepoModel() { delete rootItem; }
/**Provide access to the data through the defined QAbstractItemModel definition.
   The index parameter defines how to access the corresponded data while the
   role
   define the kind of information desired from RepoModel. This method will be
   queried
   from QView in order to populate the view.

   From the index.internalPointer the RepoModel will have access to the path,
   which uniquely define its entry on ScriptRepository.

   The RepoModel defines 4 columns:
    - Path
    - Status
    - AutoUpdate
    - Delete

   The path, provides the name of the file. The status, give information on the
   ScriptRepository
   entries. Currently, an entry may be found on the following states:
     - LOCAL_ONLY: The file only exists locally.
     - REMOTE_ONLY: The file has not been downloaded.
     - REMOTE_CHANGED: A new version of the file is available.
     - LOCAL_CHANGED: The file has been changed from the original one.
     - BOTH_CHANGED: Locally and remotelly changed.
     - UPDATED: The remote and local file are identical.

   The AutoUpdate allow to flag the entries to receive the updates automatically
   when new files
   are available at the central repository.

   The delete column will return a string "protected" or "deletable" that will
   be used to know if it
   can be deleted or not. For the current version, folders are protected, and
   files are deletable.

   The repomodel will react to the following roles:
     - DisplayRole: to provide the main information.
     - DecorationRole: to provide icons to make easier and fancier to identify
   the files and folders.
     - ToolTipRole: to provide user help to interact with this class.
 */
QVariant RepoModel::data(const QModelIndex &index, int role) const {
  using namespace Mantid::API;
  if (!index.isValid())
    return QVariant();
  auto *item = static_cast<RepoItem *>(index.internalPointer());
  try {
    const QString &path = item->path();
    Mantid::API::ScriptInfo inf;
    Mantid::API::SCRIPTSTATUS status;
    // return the data for the display role
    if (role == Qt::DisplayRole) {
      switch (index.column()) {
      case 0: // return the label (the path of the file/folder)
        return item->label();
        break;
      case 1: // ask for the status
        if (isDownloading(index))
          return downloadSt();
        if (isUploading(index))
          return uploadSt();
        status = repo_ptr->fileStatus(path.toStdString());
        return fromStatus(status);
        break;
      case 2: // autoupdate option
        status = repo_ptr->fileStatus(path.toStdString());
        if (status == REMOTE_ONLY || status == LOCAL_ONLY)
          return QVariant();
        inf = repo_ptr->fileInfo(path.toStdString());
        return inf.auto_update ? QString("true") : QString("false");
        break;
      case 3: // delete action
        inf = repo_ptr->fileInfo(path.toStdString());
        if (inf.directory)
          return PROTECTEDENTRY;
        status = repo_ptr->fileStatus(path.toStdString());
        if (!(status == LOCAL_CHANGED || status == BOTH_UNCHANGED))
          return PROTECTEDENTRY;
        return DELETABLEENTRY;
        break;
      }
    }

    // return the data for the DecorationRole
    if (role == Qt::DecorationRole) {
      if (index.column() == 0) {
        inf = repo_ptr->fileInfo(path.toStdString());
        if (inf.directory) {
          status = repo_ptr->fileStatus(path.toStdString());
          if (status == Mantid::API::REMOTE_ONLY) {
            return Icons::getIcon("mdi.folder-network-outline", "black", 1.2);
          } else
            return Icons::getIcon("mdi.folder-open-outline", "black", 1.2);
        } else {
          int pos = QString(path).lastIndexOf('.');
          if (pos < 0)
            return Icons::getIcon("mdi.file-question", "black", 1.2);
          if (path.contains("readme", Qt::CaseInsensitive))
            return Icons::getIcon("mdi.file-document-outline", "black", 1.2);

          QString extension = QString(path).remove(0, pos);
          if (extension == ".py" || extension == ".PY")
            return Icons::getIcon("mdi.language-python", "black", 1.2);
          else if (extension == ".ui")
            return Icons::getIcon("mdi.file-document-box-outline", "black", 1.2);
          else if (extension == ".docx" || extension == ".doc" || extension == ".odf")
            return Icons::getIcon("mdi.file-outline", "black", 1.2);
          else if (extension == ".pdf")
            return Icons::getIcon("mdi.file-pdf-outline", "black", 1.2);
          else
            return Icons::getIcon("mdi.file-question", "black", 1.2);
        }
      }
    } // end decorationRole

    // tool tip role
    if (role == Qt::ToolTipRole) {
      if (index.column() == 1) {
        if (isDownloading(index))
          return "Downloading... Be patient.";
        if (isUploading(index))
          return "Uploading... Be patient.";
        status = repo_ptr->fileStatus(path.toStdString());
        inf = repo_ptr->fileInfo(path.toStdString());
        switch (status) {

        case REMOTE_ONLY:
          return (inf.directory) ? "Click here to download this folder and all its files"
                                 : "Click here to download this file";
          break;
        case BOTH_UNCHANGED:
          return (inf.directory) ? "This folder is up-to-date" : "This file is up-to-date";
          break;
        case LOCAL_CHANGED:
          return "Click here to publish your changes";
        case REMOTE_CHANGED:
          return (inf.directory) ? "There is a new version of the files inside "
                                   "this folder. Click here to install them."
                                 : "There is a new version of this file "
                                   "available. Click here to install it.";
        case BOTH_CHANGED:
          return (inf.directory) ? "Files in this folder may have changed both locally and "
                                   "remotely.\nClick here to install the remote version, "
                                   "a backup of the local version will also be created."
                                 : "This file may have changed both locally and "
                                   "remotely.\nClick here to install the remote version, "
                                   "a backup of the local version will also be created.";
          break;
        case LOCAL_ONLY:
          return "Click here to share this file with the Mantid community!";
        }
      } else if (index.column() == 2) {
        return "Enable or disable this item to be downloaded automatically "
               "when new versions will be available";
      } else if (index.column() == 3) {
        if (isUploading(index))
          return "Connection busy... Be patient.";
        inf = repo_ptr->fileInfo(path.toStdString());
        if (inf.directory)
          return QVariant();
        status = repo_ptr->fileStatus(path.toStdString());
        if (!(status == LOCAL_CHANGED || status == BOTH_UNCHANGED))
          return QVariant();
        return "Click here to delete this file from the Central Repository";
      }
    } // end tool tip
  } catch (Mantid::API::ScriptRepoException &ex) {
    handleExceptions(ex, "", false);
  }
  return QVariant();
}

/** Match the Mantid::API::SCRIPTSTATUS to a string for the user to understand.
 *
 *   @param status: the SCRIPTSTATUS
 *   @return The string that defines the status.
 */
const QString &RepoModel::fromStatus(Mantid::API::SCRIPTSTATUS status) const {
  using namespace Mantid::API;
  switch (status) {
  case BOTH_UNCHANGED:
    return updatedSt();
  case REMOTE_ONLY:
    return remoteOnlySt();
  case LOCAL_ONLY:
    return localOnlySt();
  case REMOTE_CHANGED:
    return remoteChangedSt();
  case LOCAL_CHANGED:
    return localChangedSt();
  case BOTH_CHANGED:
    return bothChangedSt();
  }
  return bothChangedSt();
}

/** The ScriptRepository allows to download and upload file and folders. And to
 * configure the AutoUpdate option for the entries. These actions will be
 *available
 * through the setData method.
 *
 * The setData will recognize only the EditRole as a valid Role.
 * The RepoModel will recognize the following entries. The path can not be
 *edited
 * (column 0) is not editable. the AutoUpdate flag (column 2) will accept the
 *following
 * actions: setTrue and setFalse, to enable or disable the automatic update.
 *
 * The Status (column 1) will accept the following actions: Download and Upload.
 *
 *
 *
 */
bool RepoModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid())
    return false;
  if (role != Qt::EditRole)
    // only EditRole is acceptable for the role
    return false;
  if (index.column() == 0)
    // the path can not be changed
    return false;
  int count_changed = 0;
  auto *item = static_cast<RepoItem *>(index.internalPointer());
  std::string path = item->path().toStdString();

  bool ret = false;
  // get the action
  QString action = value.toString();
  if (index.column() == 2) { // set auto update
    bool option;
    if (action == "setTrue")
      option = true;
    else if (action == "setFalse")
      option = false;
    else
      return false; // only setTrue and setFalse are allowed values for set auto
                    // update.
    count_changed = repo_ptr->setAutoUpdate(path, option);
    ret = true;
  }

  if (index.column() == 1) { // trigger actions: Download and Upload
    if (action == "Download") {
      if (!download_threads.isFinished()) {
        QWidget *father = qobject_cast<QWidget *>(QObject::parent());
        QMessageBox::information(father, "Wait", "Downloading... ");
        return false;
      }
      downloading_path = QString::fromStdString(path);
      download_index = index;
      emit executingThread(true);
      download_threads = QtConcurrent::run(download_thread, repo_ptr, path);
      download_watcher.setFuture(download_threads);
      ret = true;
    } else if (action == "Upload") {
      if (!upload_threads.isFinished()) {
        QWidget *father = qobject_cast<QWidget *>(QObject::parent());
        QMessageBox::information(father, "Wait", "Uploading... ");
        return false;
      }

      QWidget *father = qobject_cast<QWidget *>(QObject::parent());
      if (repo_ptr->fileInfo(path).directory) {
        QMessageBox::information(father, "Not Supported",
                                 "The current version does not support "
                                 "uploading recursively. Please, upload "
                                 "one-by-one");
        return false;
      };

      auto *form = new UploadForm(QString::fromStdString(path), father);
      QSettings settings;
      settings.beginGroup("Mantid/ScriptRepository");
      QString email = settings.value("UploadEmail", QString()).toString();
      QString uploadAuthor = settings.value("UploadAuthor", QString()).toString();
      bool lastChk = settings.value("UploadSaveInfo", false).toBool();
      if (!email.isEmpty())
        form->setEmail(email);
      if (!uploadAuthor.isEmpty())
        form->setAuthor(uploadAuthor);
      form->lastSaveOption(lastChk);
      if (form->exec()) {
        settings.setValue("UploadEmail", form->saveInfo() ? form->email() : "");
        settings.setValue("UploadAuthor", form->saveInfo() ? form->author() : "");
        settings.setValue("UploadSaveInfo", form->saveInfo());

        qDebug() << "Uploading... " << QString::fromStdString(path) << form->comment() << form->author()
                 << form->email() << '\n';
        uploading_path = QString::fromStdString(path);
        upload_index = index;
        emit executingThread(true);
        upload_threads =
            QtConcurrent::run(upload_thread, repo_ptr, path, form->email(), form->author(), form->comment());
        upload_watcher.setFuture(upload_threads);
        ret = true;
      } else {
        ret = false;
      }
      settings.endGroup();
      delete form;
    }
  }

  if (index.column() == 3) { // trigger actions: delete
    using namespace Mantid::API;
    if (action != "delete")
      return false;
    // used to show qwidgets
    QWidget *father = qobject_cast<QWidget *>(QObject::parent());

    SCRIPTSTATUS status = repo_ptr->fileStatus(path);

    /* We do not remove files directly from the central repository, but,
       usually,
       this option is not available from the GUI (no button), so, just return
       false*/
    if (!(status == LOCAL_CHANGED || status == BOTH_UNCHANGED))
      return false;

    // it requires a new connection to the uploader server
    if (!upload_threads.isFinished()) {
      QWidget *mother = qobject_cast<QWidget *>(QObject::parent());
      QMessageBox::information(mother, "Wait",
                               "The connection with the server "
                               "is busy now, wait a while and "
                               "try again. ");
      return false;
    }
    // query the user if he wants to delete only locally or remote as well.
    auto box = DeleteQueryBox(QString::fromStdString(path), father);

    if (box.exec() != QMessageBox::Yes) {
      // the user gave up deleting this entry, release memory
      return false;
    }

    // get the options from the user
    QString comment(box.comment());

    // remove from central repository
    // currently, directories can not be deleted recursively
    if (repo_ptr->fileInfo(path).directory) {
      QMessageBox::information(father, "Not Supported",
                               "The current version does not support deleting "
                               "from the central repository recursively. "
                               "Please, delete one-by-one");
      return false;
    };

    // check if the reason was given and it is valid
    if (comment.isEmpty()) {
      QMessageBox::information(father, "Not Allowed", "You are not allowed to delete one file without a reason");
      return false;
    }

    // we will not allow them to remove if they have no e-mail and author saved
    QSettings settings;
    settings.beginGroup("Mantid/ScriptRepository");
    QString email = settings.value("UploadEmail", QString()).toString();
    QString uploadAuthor = settings.value("UploadAuthor", QString()).toString();
    settings.endGroup();

    if (uploadAuthor.isEmpty() || email.isEmpty()) {
      QMessageBox::information(father, "You have not uploaded this file",
                               "You are not allowed to remove files that you "
                               "have not updloaded through ScriptRepository");
      return false;
    }

    // we have all we need to delete from the central repository
    // execute the delete in a separate thread, we will use the upload
    // established way, because,
    // it will connect to the same server to delete.
    upload_index = index;
    uploading_path = QString::fromStdString(path);
    emit executingThread(true);
    upload_threads = QtConcurrent::run(delete_thread, repo_ptr, path, email, uploadAuthor, comment);
    upload_watcher.setFuture(upload_threads);
    ret = true;
  } // end delete action

  if (ret)
    emit dataChanged(index, this->index(count_changed, 0, index));

  return ret;
}

/**Define the interaction with the user allowed.
 *
 * Currently the user is allowed to select the path column, to collapse,
 * expand the folders, and he is allowed to submit actions to the columns
 * 1 and 2 (download/upload triggers) and (auto update flag).
 *
 */
Qt::ItemFlags RepoModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return {};
  if (index.column() == 0)
    return QAbstractItemModel::flags(index);
  // define that setData will accept the EditRole.
  return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

/** Return the header for the columns.
 * The RepoModel defines 4 columns with the following information:
 *  - Path
 *  - Status
 *  - AutoUpdate
 *  - Delete
 *  @param section: The column number
 *  @param orientation: It will accept only the Horizontal orientation.
 *  @param role: Only the DisplayRole will be accepted. It will not provide tool
 *tip
 *  for the headers.
 *
 *  @return The title for the columns.
 */
QVariant RepoModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (section) {
    case 0:
      return "Path";
    case 1:
      return "Status";
    case 2:
      return "AutoUpdate";
    case 3:
      return "Delete";
    default:
      return QVariant();
    }
  }
  return QVariant();
}

/** An hierarchical structure is able to define any point giving the parent, the
 *row and the
 *  column. This method is ment to be used to the View to iterate through the
 *model.
 *
 * @param row: the index of the children  (file/folder) under the given
 *folder(parent)
 * @param column: The related column (repomodel defines 3 (path, status,
 *autoupdate)
 * @param parent: The QModelIndex parent which refers to the parent folder.
 *
 * @return The QModelIndex that allows to retrieve the information of the
 *desired child.
 */
QModelIndex RepoModel::index(int row, int column, const QModelIndex &parent) const {
  // check if the row and column are allowed,
  // for example, it will not accept column == 3, or row = 1
  // for parent that refers to file and not to folder.
  if (!hasIndex(row, column, parent))
    return QModelIndex();
  // retrieve the pointer ot the RepoItem from the parent
  RepoItem *parentItem;
  if (!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<RepoItem *>(parent.internalPointer());

  // given the row, we can find the childItem from the RepoItem::child method.
  RepoItem *childItem = parentItem->child(row);

  if (childItem)
    return createIndex(row, column, childItem);
  else
    return QModelIndex();
}

/** Provide the parent of a given entry, through the QModelIndex abstraction.
 *
 * @param index: The QModelIndex that identifies one entry.
 * @return A QModelIndex that indentifies the parent of the given index.
 */
QModelIndex RepoModel::parent(const QModelIndex &index) const {
  if (!index.isValid())
    return QModelIndex();
  // the child is the RepoItem pointed by the index.
  auto *childItem = static_cast<RepoItem *>(index.internalPointer());
  // the parent is the parent of the RepoItem.
  RepoItem *parentItem = childItem->parent();
  // the root item does not have a parent
  if (parentItem == rootItem)
    return QModelIndex();
  // create the index and return
  return createIndex(parentItem->row(), 0, parentItem);
}
/** Count how many file/folders are direct children of the given folder,
 * through the abstraction of QModelIndex.
 *
 * @param parent: the index to the folder.
 * @return the number of children of the given folder.
 */
int RepoModel::rowCount(const QModelIndex &parent) const {
  RepoItem *parentItem;

  if (parent.column() > 0)
    return 0; // there are rows defined only of the column 0

  if (!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<RepoItem *>(parent.internalPointer());
  // return the number of children
  return parentItem->childCount();
}

/** Return the number of columns defined for the given index.
 * But, for all the index, the number of columns will be always 4.
 * (path, status, autoupdate, delete)
 *
 * @return 4.
 */
int RepoModel::columnCount(const QModelIndex & /*parent*/) const { return 4; }

/** Return the description of the file for a defined entry
 **/
QString RepoModel::fileDescription(const QModelIndex &index) {
  auto *item = static_cast<RepoItem *>(index.internalPointer());
  if (!item)
    return "";
  QString desc;
  try {
    desc = QString::fromStdString(repo_ptr->description(item->path().toStdString()));
  } catch (...) {
    // just ignore
  }
  return desc;
}

QString RepoModel::author(const QModelIndex &index) {
  auto *item = static_cast<RepoItem *>(index.internalPointer());
  QString author = "Not defined";
  if (!item)
    return author;
  try {
    author = QString::fromStdString(repo_ptr->info(item->path().toStdString()).author);
  } catch (...) {
    // just ignore
  }
  return author;
}
/** Return the operative system file path if it exists.
    otherwise it returns an empty string
    @param index: to find the entry
    @return The operative system path or empty string
*/
QString RepoModel::filePath(const QModelIndex &index) {
  auto *item = static_cast<RepoItem *>(index.internalPointer());
  //   qDebug() << "Get file path from : " <<  item->path()<< '\n';
  Mantid::API::SCRIPTSTATUS state = repo_ptr->fileStatus(item->path().toStdString());

  if (state == Mantid::API::REMOTE_ONLY)
    return "";
  Mantid::API::ScriptInfo info = repo_ptr->fileInfo(item->path().toStdString());
  if (info.directory)
    return "";
  QString path = repo_path + "/" + item->path();
  return path;
}

/**Auxiliary method to help setupModelData to populate all the
   entries of RepoModel.

   For any entry of ScriptRepository should have a parent (by definition, all
the entries
   are children of the root entry). Besides, given an entry, if the path
contains a '/' this
   means that the entry is child of one folder. And all the folders are parents
by definition.

   So, the idea of this getParent is that given a folder name, it must be
related to a RepoItem
   that is a parent. If it does not exists, it should be created.

   @param folder: relative path inside the repository for the folder.
   @param parents Reference to the list of parents
   @return Pointer to the RepoItem related to the given folder.

**/
RepoModel::RepoItem *RepoModel::getParent(const QString &folder, QList<RepoItem *> &parents) {
  // in order to speed the algorithm, check if the
  // folder is the same of the last folder.
  if (parents.last()->path() == folder)
    return parents.last();

  // try to find this
  if (folder.isEmpty())
    return parents.first(); // the parents first will always contain the root

  // it will iterate through all the parents of the given folder, in order to
  // create any folder that has not been created.
  QStringList folder_parts = folder.split("/");
  QString aux_folder;
  RepoItem *father = parents.first();
  // there is no reason to try to find entry A/B/C if the entry A/B was not
  // found
  bool try_to_find = true;

  for (int i = 0; i < folder_parts.size(); i++) {
    if (i == 0)
      aux_folder = folder_parts[i];
    else
      aux_folder += "/" + folder_parts[i];

    bool found = false;

    if (try_to_find) {
      // this means that the previous folders were found
      const auto it = std::find_if(parents.cbegin(), parents.cend(),
                                   [&aux_folder](const auto &parent) { return parent->path() == aux_folder; });
      if (it != parents.cend()) {
        found = true;
        father = *it;
      }
    }
    // there is not RepoItem related to the current folder,
    // create it
    if (!found) {
      RepoItem *m = new RepoItem(folder_parts[i], aux_folder, father);
      father->appendChild(m);
      parents.append(m);
      father = m;
      try_to_find = false;
    }
  }
  return father;
}

/** Populate the RepoModel with RepoItem entries that reproduce the hierarchical
 *  organization of ScriptRepository entries.
 *
 * It will get the information from ScriptRepository from the listFiles, which
 *means,
 * that it will reconstruct the hierarchical organization of the files and
 *folders
 * through the list of strings with the relative path of each entry.
 *
 * @param root: The RepoItem root
 */
void RepoModel::setupModelData(RepoItem *root) {
  // check server for updates to repository
  repo_ptr->check4Update();
  // get the list of entries inside the scriptrepository
  std::vector<std::string> list = repo_ptr->listFiles();

  // auxiliary list of pointers to repoitem that are related to folders
  QList<RepoItem *> parents;
  // the first one will always be the root
  parents << root;

  // FOREACH entry in LISTFILES
  for (const auto &number : list) {
    // folder or file inside the repository
    QString lineData = QString::fromStdString(number);

    // Read the column data from the rest of the line.
    QStringList pathStrings = lineData.split("/");
    // separate the folder and the current entry (folder or file)
    QString current_file = pathStrings.last();
    QString folder = "";
    pathStrings.removeLast();
    if (pathStrings.size() > 0)
      folder = pathStrings.join("/");

    // get parent for this entry
    RepoItem *parentOfFolder = getParent(folder, parents);
    // a new folder has started
    if (parentOfFolder == root) {
      // this test is just for the sake of performance, to reduce the numbers of
      // parents
      parents.clear();
      parents << root;
    }

    // check if the current entry is a directory
    if (repo_ptr->info(lineData.toStdString()).directory) {
      // directories will be appended to parents list
      RepoItem *aux = new RepoItem(current_file, lineData, parentOfFolder);
      parentOfFolder->appendChild(aux);
      parents << aux;
    } else {
      // files will just be created and appended to the parent
      parentOfFolder->appendChild(new RepoItem(current_file, lineData, parentOfFolder));
    }
  }
}

void RepoModel::handleExceptions(const Mantid::API::ScriptRepoException &ex, const QString &title,
                                 bool showWarning) const {
  g_log.information() << "Download failed " << ex.what() << "\n Detail: " << ex.systemError() << '\n';
  if (showWarning) {

    QWidget *father = qobject_cast<QWidget *>(QObject::parent());
    QString info = QString::fromStdString(ex.what());
    // make the exception a nice html message
    info.replace("\n", "</p><p>");
    QMessageBox::warning(father, title, QString("<html><body><p>%1</p></body></html>").arg(info));
  }
}

void RepoModel::downloadFinished(void) {
  QString info = download_threads.result();
  if (!info.isEmpty()) {
    QMessageBox::warning(qobject_cast<QWidget *>(QObject::parent()), "Download Failed",
                         QString("<html><body><p>%1</p></body></html>").arg(info));
  }
  downloading_path = nofile_flag;
  auto *repo_item = static_cast<RepoItem *>(download_index.internalPointer());
  QModelIndex top_left = createIndex(0, 0, repo_item);
  QModelIndex bottom_right = createIndex(0, 3, repo_item);
  emit dataChanged(top_left, bottom_right);
  emit executingThread(false);
}

bool RepoModel::isDownloading(const QModelIndex &index) const {
  const auto *item = static_cast<RepoItem *>(index.internalPointer());
  if (item)
    return item->path() == downloading_path;
  return false;
}

void RepoModel::uploadFinished(void) {
  QString info = upload_threads.result();
  QString title = "Upload Failed";
  if (info.contains(delete_mark)) {
    info.replace(delete_mark, "");
    title = "Delete Failed";
  }

  if (!info.isEmpty()) {
    QMessageBox::warning(qobject_cast<QWidget *>(QObject::parent()), title,
                         QString("<html><body><p>%1</p></body></html>").arg(info));
  }

  uploading_path = nofile_flag;
  auto *repo_item = static_cast<RepoItem *>(upload_index.internalPointer());
  QModelIndex top_left = createIndex(0, 0, repo_item);
  QModelIndex bottom_right = createIndex(0, 3, repo_item);
  emit dataChanged(top_left, bottom_right);
  emit executingThread(false);
}

bool RepoModel::isUploading(const QModelIndex &index) const {
  const auto *item = static_cast<RepoItem *>(index.internalPointer());
  if (item)
    return item->path() == uploading_path;
  return false;
}

/// @return string to define the LOCAL_ONLY state
const QString &RepoModel::localOnlySt() { return LOCALONLY; }
/// @return string to define the REMOTE_ONLY state
const QString &RepoModel::remoteOnlySt() { return REMOTEONLY; }
/// @return string to define the LOCAL_CHANGED state
const QString &RepoModel::localChangedSt() { return LOCALCHANGED; }
/// @return string to define the REMOTE_CHANGED state
const QString &RepoModel::remoteChangedSt() { return REMOTECHANGED; }
/// @return string to define the BOTH_UNCHANGED state
const QString &RepoModel::updatedSt() { return BOTHUNCHANGED; }
/// @return string to define the BOTH_CHANGED state
const QString &RepoModel::bothChangedSt() { return BOTHCHANGED; }
/// @return string to define the downloading state
const QString &RepoModel::downloadSt() { return DOWNLOADST; }
/// @return string to define the uploading state
const QString &RepoModel::uploadSt() { return UPLOADST; }

RepoModel::UploadForm::UploadForm(const QString &file2upload, QWidget *parent) : QDialog(parent) {
  author_le = new QLineEdit();
  email_le = new QLineEdit();
  save_ck = new QCheckBox("Save your personal information");
  save_ck->setToolTip("The author and email will be saved and will be written "
                      "to you next time");
  comment_te = new QTextEdit();

  // setup the layout

  auto *personalGroupBox = new QGroupBox("Personal Group Box");
  auto *personalLayout = new QFormLayout();
  personalLayout->addRow("Author", author_le);
  personalLayout->addRow("Email", email_le);
  auto *gpBox = new QVBoxLayout();
  gpBox->addWidget(save_ck);
  gpBox->addLayout(personalLayout);
  personalGroupBox->setLayout(gpBox);

  QLabel *cmLabel = new QLabel("Comment");
  auto *buttonBox = new QDialogButtonBox();
  buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

  auto *layout = new QVBoxLayout();
  layout->addWidget(personalGroupBox);
  layout->addWidget(cmLabel);
  layout->addWidget(comment_te);
  layout->addWidget(buttonBox);
  setLayout(layout);

  setWindowTitle(QString("Upload - %2").arg(file2upload));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}
RepoModel::UploadForm::~UploadForm() = default;
QString RepoModel::UploadForm::email() { return email_le->text(); }
QString RepoModel::UploadForm::author() { return author_le->text(); }
QString RepoModel::UploadForm::comment() { return comment_te->toPlainText(); }
bool RepoModel::UploadForm::saveInfo() { return save_ck->isChecked(); }
void RepoModel::UploadForm::setEmail(const QString &email) { email_le->setText(email); }
void RepoModel::UploadForm::setAuthor(const QString &author) { author_le->setText(author); }
void RepoModel::UploadForm::lastSaveOption(bool option) {
  save_ck->setCheckState(option ? Qt::Checked : Qt::Unchecked);
}

RepoModel::DeleteQueryBox::DeleteQueryBox(const QString &path, QWidget *parent)
    : QMessageBox(QMessageBox::Question, "Delete file", "", QMessageBox::Yes | QMessageBox::No, parent) {
  using namespace Mantid::API;
  QString info_str;
  QTextStream info(&info_str);

  info << "<html><head/><body><p>Are you sure you want to delete this file "
          "from the Repository?</p><p align=\"center\"><span style=\" "
          "font-style:italic;\">"
       << path << "</span></p></body></html>";

  // creation of the new widgets
  comment_te = nullptr;

  setText(info_str);

  QGridLayout *_lay = qobject_cast<QGridLayout *>(layout());
  if (_lay) {
    QLayoutItem *buttons = _lay->takeAt(_lay->count() - 1);
    QLabel *la = new QLabel("Please, give the reason for deleting:", this);
    comment_te = new QTextEdit(this);
    comment_te->setMaximumHeight(70);
    _lay->addWidget(la, _lay->rowCount(), 0, 1, -1);
    _lay->addWidget(comment_te, _lay->rowCount(), 0, 2, -1);
    _lay->addItem(buttons, _lay->rowCount(), 0, 1, -1);
  }
}

RepoModel::DeleteQueryBox::~DeleteQueryBox() = default;
QString RepoModel::DeleteQueryBox::comment() {
  if (comment_te)
    return comment_te->toPlainText();
  else
    return QString();
}
