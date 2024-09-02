// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ScriptRepositoryView.h"
#include "MantidAPI/ScriptRepository.h"
#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "MantidQtWidgets/Common/RepoModel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTime>
#include <QVBoxLayout>

namespace MantidQt::API {
namespace {
/// static logger
Mantid::Kernel::Logger g_log("ScriptRepositoryView");
} // namespace

const QString install_mantid_label =
    "<html><head/><body><p>The <span style=\" font-weight:600;\">"
    "Script Repository</span> allows you to:</p>"
    "<p>  * Share your scripts and reduction algorithms;</p>"
    "<p>  * Get <span style=\" font-weight:600;\">Mantid</span> Scripts from "
    "the mantid developers and the community. </p>"
    "<p><span style=\" font-style:italic;\">"
    "N.B. The installation usually requires a couple of minutes, depending on "
    "your network bandwidth. </span></p>"
    "<p>More Information available at "
    "<a href=\"http://docs.mantidproject.org/workbench/scriptrepository\"><span style=\" "
    "text-decoration: underline; color:#0000ff;\">"
    "http://docs.mantidproject.org/workbench/scriptrepository</span></a></p></br><p><span "
    "style=\" font-weight:600;\">"
    "Would you like to install it now?</span></p></body></html>";

const QString installation_in_progress = "<html><head/><body><p><span style=\" font-weight:600;\">"
                                         "Installing Script Repository Installation in background!</span></p>"
                                         "<p>You may continue to use mantid.</p>"
                                         "<p>The Result Log willl give you information of the installation "
                                         "progress.</p>"
                                         "<p>When finished, please, reopen the <span style=\" "
                                         "font-weight:600;\">Script Repository</span>. </p></body></html>";

const QString installation_failed = "<html><head/><body><p>The installation of Script Repository "
                                    "<span style=\" font-weight:600;\">Failed</span>!</p>"
                                    "<p>Please, check the Result Log to see why the installation failed. "
                                    "</p></body></html>";

const QString dir_not_empty_label = "<html><head/><body><p>The directory/folder that you have selected is not "
                                    "empty</p>"
                                    "<p>Are you sure that you want to install the script repository here? All "
                                    "the files and directories found in "
                                    "the selected directory/folder could be shared in the repository by "
                                    "mistake.</p>"
                                    "<p>If you are not sure, please choose 'no' and then select an empty (or "
                                    "newly created) directory/folder.</p>"
                                    "<p>If this is your home directory, desktop or similar you should "
                                    "definitely choose 'no'.</p>"
                                    "<p>If you are sure of what you are doing, please choose 'yes'. The "
                                    "installation may take a couple of minutes.</p>"
                                    "</body></html>";

//----------------------------------------------------------------------------------------------
/** Creates the widget for the ScriptRepositoryView
 *
 *  Before constructing the widget, it must ensure that the ScriptRepository was
 *installed before.
 *
 *  If it has not been installed, them, it will first try to install it.
 *  If it fails to install, them, it will not be able to create the widget, and
 *it will fail gracelly.
 *
 *  In normal condition, it will create the widget (Ui::ScriptRepositoryView)
 *and populate it with the
 *  RepoModel, and define the delegates ScriptRepositoryView::RepoDelegate and
 *ScriptRepositoryView::CheckBoxDelegate
 *  and ScriptRepositoryView::RemoveEntryDelegate.
 *
 */
ScriptRepositoryView::ScriptRepositoryView(QWidget *parent) : MantidDialog(parent), ui(new Ui::ScriptRepositoryView) {
  using Mantid::API::ScriptRepositoryFactory;
  using Mantid::Kernel::ConfigService;
  using Mantid::Kernel::ConfigServiceImpl;

  ui->setupUi(this);

  try {
    // create and instance of ScriptRepository
    Mantid::API::ScriptRepository_sptr repo_ptr = ScriptRepositoryFactory::Instance().create("ScriptRepositoryImpl");

    // check if the ScriptRepository was ever installed
    if (!repo_ptr->isValid()) {
      bool successful = chooseLocationAndInstall(repo_ptr);
      if (!successful) {
        return;
      }
    }
    // create the model
    model = new RepoModel(this);

  } catch (Mantid::API::ScriptRepoException &ex) {
    // means that the installation failed
    g_log.warning() << "ScriptRepository installation: " << ex.what() << '\n';
    g_log.information() << "ScriptRepository installation failed with this information: " << ex.systemError() << '\n';
    QMessageBox::warning(this, "Installation Failed", QString(ex.what()));
    close();
    deleteLater();
    return;
  } catch (...) {
    g_log.error() << "Unknown error occurred to install ScriptRepository. It "
                     "will not be shown.\n";
    close();
    deleteLater();
    return;
  }
  // from this point, it is assumed that ScriptRepository is installed.

  // configure the Ui
  connect(ui->reloadPushButton, SIGNAL(clicked()), this, SLOT(updateModel()));
  connect(ui->pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(model, SIGNAL(executingThread(bool)), ui->reloadPushButton, SLOT(setDisabled(bool)));

  // setup the model and delegates
  ui->repo_treeView->setModel(model);
  ui->repo_treeView->setItemDelegateForColumn(1, new RepoDelegate(this));
  ui->repo_treeView->setItemDelegateForColumn(2, new CheckBoxDelegate(this));
  ui->repo_treeView->setItemDelegateForColumn(3, new RemoveEntryDelegate(this));
  ui->repo_treeView->setColumnWidth(0, 290);

  // stablish the connections.
  connect(ui->repo_treeView, SIGNAL(activated(const QModelIndex &)), this, SLOT(cell_activated(const QModelIndex &)));
  connect(ui->repo_treeView, SIGNAL(currentCell(const QModelIndex &)), this, SLOT(currentChanged(const QModelIndex &)));
  const ConfigServiceImpl &config = ConfigService::Instance();
  updateLocationString(config.getString("ScriptLocalRepository"));
  connect(ui->folderPathLabel, SIGNAL(linkActivated(QString)), this, SLOT(openFolderLink(QString)));
}

/**
 * @brief Prompt the user with where to install the script repository, and install it.
 * Closes the view if the user declines to install.
 * @param repo_ptr ScriptRepository pointer to be used to install the script repo
 * @return true if successfully installed, false if not
 */
bool ScriptRepositoryView::chooseLocationAndInstall(Mantid::API::ScriptRepository_sptr repo_ptr) {
  using Mantid::Kernel::ConfigService;
  using Mantid::Kernel::ConfigServiceImpl;
  enum EXC_OPTIONS { NOTWANTED, NODIRECTORY };

  QString dir;
  try {
    // Ask the user if he wants to install the ScriptRepository
    if (QMessageBox::Ok != QMessageBox::question(this, "Install Script Repository?", install_mantid_label,
                                                 QMessageBox::Ok | QMessageBox::Cancel)) {
      throw NOTWANTED;
    }
    // get the directory to install the script repository
    const ConfigServiceImpl &config = ConfigService::Instance();
    QString loc = QString::fromStdString(config.getString("ScriptLocalRepository"));

    bool sureAboutDir = false;

    while (!sureAboutDir) {
      dir = QFileDialog::getExistingDirectory(this, tr("Where do you want to install Script Repository?"), loc,
                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

      // configuring
      if (dir.isEmpty()) {
        throw NODIRECTORY;
      }

      // warn if dir is not empty
      if (0 == QDir(dir).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot).count()) {
        // empty dir, just go ahead
        sureAboutDir = true;
      } else {
        // warn user in case the repo is being installed in its home, etc.
        // directory
        QMessageBox::StandardButton sel =
            QMessageBox::question(this, "Are you sure you want to install the Script Repository here?",
                                  dir_not_empty_label, QMessageBox::Yes | QMessageBox::No);
        if (QMessageBox::Yes == sel)
          sureAboutDir = true;
      }
    }
  } catch (EXC_OPTIONS &ex) {
    if (ex == NODIRECTORY) {
      // probably the user change mind. He does not want to install any more.
      QMessageBox::warning(this, "Installation Failed", "Invalid Folder to install Script Repository!\n");
    }

    close();
    deleteLater();
    return false;
  }
  // attempt to install
  repo_ptr->install(dir.toStdString());
  updateLocationString(dir.toStdString());
  g_log.information() << "ScriptRepository installed at " << dir.toStdString() << '\n';
  return true;
}

void ScriptRepositoryView::updateLocationString(const std::string &installDir) {
  const QString loc = QString::fromStdString(installDir);
  const QString loc_info = "<html><head/><body><p><a href=\"%1\"><span style=\" "
                           "text-decoration: underline; "
                           "color:#0000ff;\">%2</span></a></p></body></html>";
  QString path_label;
  if (loc.size() < 50)
    path_label = loc;
  else {
    path_label = QString("%1...%2").arg(loc.left(20)).arg(loc.right(27));
  }

  ui->folderPathLabel->setText(loc_info.arg(loc).arg(path_label));
  ui->folderPathLabel->setToolTip(QString("Click here to open Script Repository Folder: %1.").arg(loc));
}

/** This method refreshes the ScriptRepository and allows it
 *  to check list the files again. It will also check for
 *  new files and folders. It is easier to just recreate RepoModel
 *  than figuring out the entries that were inserted or deleted
 *  from the ScriptRepository. This method could be rewritten
 *  in order to be more efficient.
 */
void ScriptRepositoryView::updateModel() {
  RepoModel *before = model;
  auto model_repo_ptr = model->getRepoPtr();
  if (!model_repo_ptr->isValid()) {
    const bool success = chooseLocationAndInstall(model_repo_ptr);
    if (!success) {
      return;
    }
  }
  model = new RepoModel();
  connect(model, SIGNAL(executingThread(bool)), ui->reloadPushButton, SLOT(setDisabled(bool)));
  ui->repo_treeView->setModel(model);
  delete before;
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ScriptRepositoryView::~ScriptRepositoryView() { delete ui; }

/** Allows the user to open a file to investigate it.
 *  If the user selects and activate one Row, double-clicking on the first
 *  column, it will try to retrieve the file path (if it is local) and emit
 *  the signal loadScript. Mantid will get this signal to load the file and
 *  show its contents to the user.
 */
void ScriptRepositoryView::cell_activated(const QModelIndex &in) {

  RepoModel *_model = qobject_cast<RepoModel *>(ui->repo_treeView->model());
  if (_model) {
    QString path = _model->filePath(in);
    if (path.isEmpty()) {
      // no real file to be opened.
      return;
    }
    emit loadScript(path);
  }
}

/** This method will be executed every time the user change the selection. It
 * allows
 *  to update all the entries that are related to the current selection.
 * Currently,
 *  the description field will be updated.
 */
void ScriptRepositoryView::currentChanged(const QModelIndex &in) {
  RepoModel *_model = qobject_cast<RepoModel *>(ui->repo_treeView->model());
  if (_model) {
    // try to get the description of the file pointed at the current index.
    // and update the description text browser.
    QString description = _model->fileDescription(in);
    ui->desc_textBrowser->setText(description);
    QString author_name = _model->author(in);
    if (author_name.isEmpty())
      ui->authorNameLabel->setText("");
    else
      ui->authorNameLabel->setText(QString("<b>Author:</b> ") + author_name);
    return;
  }
}

/** Open the ScriptRepository Page on Web Browser*/
void ScriptRepositoryView::helpClicked() {
  HelpWindow::showPage(QStringLiteral("qthelp://org.mantidproject/doc/workbench/scriptrepository.html"));
}

//////////////////////////////////////////////////
// DELEGATE : Allow to display and interact with the View in a nicer way.
// Improve the User Experience.
///////////////////////////////////////////////////

ScriptRepositoryView::RepoDelegate::RepoDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
/** Draws the column 1 (Status) of ScriptRepositoryView.
 *
 *  This function is called every time the ScriptRepository needs to
 *draw the widget for the Status of the file/folder inside the
 *ScriptRepository.  Instead of displaying the status (REMOTE_ONLY,
 *LOCAL_ONLY, and so on), it will draw an Icon that 'hoppefully' will
 *better indicate to the user the condition of the entry as well as
 *encourage him to act. The action will be dealt with through the
 *editorEvent.
 *
 *  When this method is called, it will get the index in order to
 *retrieve the information about the status of the entry
 *(folder/file).
 *
 *  It will them decide which icon better describes the current status
 *of the entry, and will draw it using the option and the painter
 *given.
 *
 * @param painter: Required to draw the widget
 * @param option: Provided by the framework and has information
 *displaying the widget.
 * @param index: Identifies the entry inside the RepoModel (indirectly
 * the file/folder).
 */
void ScriptRepositoryView::RepoDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                               const QModelIndex &index) const {

  if (!index.isValid())
    return;
  if (painter->device() == nullptr)
    return;

  // get the state and chose the best fit icon
  QString state = index.model()->data(index, Qt::DisplayRole).toString();
  auto icon = getIcon(state);

  // define the region to draw the icon
  QRect buttonRect(option.rect);
  int min_val = buttonRect.width() < buttonRect.height() ? buttonRect.width() : buttonRect.height();
  // make it square
  buttonRect.setWidth(min_val);
  buttonRect.setHeight(min_val);
  buttonRect.moveCenter(option.rect.center());

  // define the options to draw a push button with the icon displayed
  QStyleOptionButton button;
  button.rect = buttonRect;
  button.icon = icon;
  int icon_size = (int)(min_val * .8);
  button.iconSize = QSize(icon_size, icon_size);
  button.state = QStyle::State_Enabled;
  // draw a push button
  QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter);
}

QIcon ScriptRepositoryView::RepoDelegate::getIcon(const QString &state) const {
  QIcon icon;
  if (state == RepoModel::remoteOnlySt())
    icon = Icons::getIcon("mdi.download");
  else if (state == RepoModel::remoteChangedSt() || state == RepoModel::bothChangedSt()) {
    icon = Icons::getIcon("mdi.transfer-down");
  } else if (state == RepoModel::updatedSt())
    icon = Icons::getIcon("mdi.check-bold");
  else if (state == RepoModel::localOnlySt() || state == RepoModel::localChangedSt())
    icon = Icons::getIcon("mdi.upload");
  else if (state == RepoModel::downloadSt())
    icon = Icons::getIcon("mdi.progress-download");
  else if (state == RepoModel::uploadSt())
    icon = Icons::getIcon("mdi.progress-upload");
  return icon;
}

/** Reacts to the iteraction with the user when he clicks on the buttons
 *displayed at paint.
 *
 *  Given the state of an entry (folder/file) there is only on available action.
 *So,
 *  it is enough to get the event that the user interact with the pushbutton to
 *decide what
 *  to do.
 *
 *  It will filter the event in order to get the Left-Click of mouse. If it gets
 *the
 *  click of the mouse, it will trigger the action:
 *   - Upload: if the file/folder is local_only or local_changed
 *   - No Action when the entry is in Updated state.
 *   - Download: for the other cases
 *
 * @param event: The event given by the framework
 * @param model: Pointer to the model needed to retrive the status of the entry
 * @param index: identifies the entry (file/folder)
 * @param option: Provided by the framewor, and passed on to the base class.
 * @return true if it handles or false to ignore.
 */
bool ScriptRepositoryView::RepoDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                                     const QStyleOptionViewItem & /*option*/,
                                                     const QModelIndex &index) {
  // if event is mouse click
  if (event->type() == QEvent::MouseButtonPress) {
    QString value = model->data(index, Qt::DisplayRole).toString();
    QString action = "Download";
    if (value == RepoModel::localOnlySt() || value == RepoModel::localChangedSt())
      action = "Upload";
    if (value == RepoModel::updatedSt())
      return false; // ignore
    return model->setData(index, action, Qt::EditRole);
  } else {
    return true; // Does not allow others events to be processed (example:
                 // double-click)
  }
}
/** Provides the ideal size for this column
 *  @return ideal size for this column
 */
QSize ScriptRepositoryView::RepoDelegate::sizeHint(const QStyleOptionViewItem & /*option*/,
                                                   const QModelIndex & /*index*/) const {
  return QSize(35, 35);
}

//////////////////////////////////////////////////
// CheckBoxDelegate
///////////////////////////////////////////////////

ScriptRepositoryView::CheckBoxDelegate::CheckBoxDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
/** Draws the column 2 (AutoUpdate) of ScriptRepositoryView.
 *
 *  This function is called every time the ScriptRepository needs to
 *draw the widget for the AutoUpdate of the file/folder inside the
 *ScriptRepository.  Instead of displaying the strings 'true' and
 *'false' it will draw a checkbox that 'hoppefully' will better
 *indicate to the user the condition of the entry as well as
 *encourage him to act. The action will be dealt with at the
 *editorEvent.
 *
 *  When this method is called, it will get the index in order to
 *retrieve the information about the state of the entry
 *(folder/file).
 *
 *
 * @param painter: Required to draw the widget
 * @param option: Provided by the framework and has information
 *displaying the widget.
 * @param index: Identifies the entry inside the RepoModel
 * (indirectly the file / folder).
 */
void ScriptRepositoryView::CheckBoxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                                   const QModelIndex &index) const {
  if (!index.isValid())
    return;
  if (painter->device() == nullptr)
    return;

  QStyleOptionViewItem modifiedOption(option);

  QPoint p = modifiedOption.rect.center();
  QSize curr = modifiedOption.rect.size();
  int min_value = (int)((curr.width() < curr.height()) ? curr.width() : curr.height() * .8);
  // make the checkbox a square in the center of the cell
  modifiedOption.rect.setSize(QSize(min_value, min_value));
  modifiedOption.rect.moveCenter(p);
  // get the current state of this entry
  QString state = index.model()->data(index, Qt::DisplayRole).toString();

  if (state == "true")
    modifiedOption.state |= QStyle::State_On;
  else if (state == "false")
    modifiedOption.state |= QStyle::State_Off;
  else
    return;
  // draw it
  QApplication::style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &modifiedOption, painter);
}

/** Reacts to the iteraction with the user when he clicks on the buttons
 *displayed at paint.
 *
 *  Given the state of an entry (folder/file) there is only on available action.
 *So,
 *  it is enough to get the event that the user interact with the checkbox to
 *decide what
 *  to do.
 *
 *  It will filter the event in order to get the Left-Click of mouse. If it gets
 *the
 *  click of the mouse, it will trigger the action to toggle the state of the
 *checkbox,
 *  which means, trigger the action 'setTrue' if the current state is 'false' of
 *  trigger the action 'setFalse' if the current state is 'true'.
 *
 * @param event: The event given by the framework
 * @param model: Pointer to the model needed to retrive the status of the entry
 * @param index: identifies the entry (file/folder)
 * @param option: Provided by the framewor, and passed on to the base class.
 * @return true if it handles or false to ignore.
 */
bool ScriptRepositoryView::CheckBoxDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                                         const QStyleOptionViewItem & /*option*/,
                                                         const QModelIndex &index) {
  if (event->type() == QEvent::MouseButtonPress) {
    QString value = model->data(index, Qt::DisplayRole).toString();
    QString action = "setFalse";
    if (value == "false")
      action = "setTrue";
    return model->setData(index, action, Qt::EditRole);
  } else {
    // QStyledItemDelegate::editorEvent(event, model, option, index);
    return true; // Does not allow the event to be catched by another one
  }
}
/////////////////////
// RemoveEntryDelegate
/////////////////////

ScriptRepositoryView::RemoveEntryDelegate::RemoveEntryDelegate(QObject *parent) : QStyledItemDelegate(parent) {}
/** Draws the column 3 (delete) of ScriptRepositoryView.
 *
 *  This function is called every time the ScriptRepository needs to
 *draw the widget for the delete column of the file/folder inside the
 *ScriptRepository.  It displays a trash icon to indicate user that it
 *is used to remove entries.
 *
 * @param painter: Required to draw the widget
 * @param option: Provided by the framework and has information
 *displaying the widget.
 * @param index: Identifies the entry inside the RepoModel (indirectly
 * the file / folder).
 */
void ScriptRepositoryView::RemoveEntryDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                                      const QModelIndex &index) const {

  if (!index.isValid())
    return;
  if (painter->device() == nullptr)
    return;

  QIcon icon;
  // get the state and chose the best fit icon
  QString entry_type = index.model()->data(index, Qt::DisplayRole).toString();

  if (entry_type == "protected")
    return;

  icon = Icons::getIcon("mdi.trash-can");

  // define the region to draw the icon
  QRect buttonRect(option.rect);
  int min_val = buttonRect.width() < buttonRect.height() ? buttonRect.width() : buttonRect.height();
  // make it square
  buttonRect.setWidth(min_val);
  buttonRect.setHeight(min_val);
  buttonRect.moveCenter(option.rect.center());

  // define the options to draw a push button with the icon displayed
  QStyleOptionButton button;
  button.rect = buttonRect;
  button.icon = icon;
  int icon_size = (int)(min_val * .8);
  button.iconSize = QSize(icon_size, icon_size);
  button.state = QStyle::State_Enabled;
  // draw a push button
  QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter);
}

/** Reacts to the iteraction with the user when he clicks on the buttons
 *displayed at paint.
 *
 *  Clicking on the delete icon there is only on available action (to delete the
 *entry). So,
 *  it is enough to get the event that the user interact with the pushbutton to
 *decide what
 *  to do.
 *
 *  It will filter the event in order to get the Left-Click of mouse. If it gets
 *the
 *  click of the mouse, it will trigger the action delete to the model
 *
 * @param event: The event given by the framework
 * @param model: Pointer to the model needed to retrive the status of the entry
 * @param index: identifies the entry (file/folder)
 * @param option: Provided by the framewor, and passed on to the base class.
 * @return true if it handles or false to ignore.
 */
bool ScriptRepositoryView::RemoveEntryDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                                                            const QStyleOptionViewItem & /*option*/,
                                                            const QModelIndex &index) {
  // if event is mouse click
  if (event->type() == QEvent::MouseButtonPress) {
    QString entry = index.model()->data(index, Qt::DisplayRole).toString();
    if (entry == "protected")
      return true;
    QString action = "delete";
    return model->setData(index, action, Qt::EditRole);
  } else {
    return true; // Does not allow others events to be processed (example:
                 // double-click)
  }
}

/**
 * Attempt to open the given folder link using an appropriate application.
 *
 * @param link :: the folder link to open.
 */
void ScriptRepositoryView::openFolderLink(const QString &link) {
  const std::string error_msg = "Unable to open \"" + link.toStdString() + "\".  Reason: ";

  // QUrl::fromLocalFile seems to be the most robust way of constructing QUrls
  // on
  // the local file system for all platforms.
  const QUrl url = QUrl::fromLocalFile(link);
  if (!url.isValid()) {
    g_log.error() << error_msg << "Invalid (malformed) URL.\n";
    return;
  }

  const bool openSuccessful = MantidDesktopServices::openUrl(url);
  if (!openSuccessful)
    g_log.error() << error_msg << "Could not find directory.\n";
}

} // namespace MantidQt::API
