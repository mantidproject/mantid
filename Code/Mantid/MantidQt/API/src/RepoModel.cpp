#include <QtGui>
#include "MantidQtAPI/RepoModel.h"

#include "MantidAPI/ScriptRepositoryFactory.h"
#include <vector>

#include "MantidKernel/ConfigService.h"
#include <QIcon>
#include <QPixmap>
using namespace MantidQt::API;
using Mantid::Kernel::ConfigServiceImpl; 
using Mantid::Kernel::ConfigService;
#include <QDebug>
/*
    repoitem.cpp

    A container for items of data supplied by the script repository model.
*/

RepoItem::RepoItem(const QString label, const QString path, 
             bool entry_directory,
             Mantid::API::SCRIPTSTATUS curr_status, 
             RepoItem * parent)
{
  if (!parent)
    root = true;
  else
    root = false; 

  parentItem = parent;
  this->label = label; 
  this->path = path; 
  this->status = curr_status; 
  this->directory = entry_directory; 
}
RepoItem::~RepoItem()
{
    qDeleteAll(childItems);
}

void RepoItem::appendChild(RepoItem *child)
{
    childItems.append(child);
}

RepoItem *RepoItem::child(int row)
{  
    return childItems.value(row);
}

int RepoItem::childCount() const
{
    return childItems.count();
}

int RepoItem::columnCount() const
{  
   return 5;
}

QVariant RepoItem::data(int column,  int role  ) const
{
  if (root){
    if (role != Qt::DisplayRole)
      return QVariant(); 
    switch(column){
    case 0:
      return "File"; 
    case 1:
      return "Download";
    case 2:
      return "Update"; 
    case 3: 
      return "Publish"; 
    default: 
      return "Path";
    }
  }

  switch(role){
  case  Qt::DisplayRole:
    {
      switch(column){
      case 0:
        return label;
        break;
      case 1: /*Donwload -> is this file already donwloaded?*/           
        if (status == Mantid::API::REMOTE_ONLY)
          return "false";
        else{
          /*if the file is only local, this question does not apply*/
          if (status == Mantid::API::LOCAL_ONLY)
            return "";
          
        }
        return "true";
        break;
      case 2:/* Update -> The file is up to date?*/ 
        if (status ==  Mantid::API::BOTH_UNCHANGED)
          return "true";
        else{
          /* If the file was not downloaded, them, this question does not apply*/
          if (status ==  Mantid::API::REMOTE_ONLY
              || 
              status == Mantid::API::LOCAL_ONLY)
            return ""; 
          else
            return "false";
        }
        break;
      case 3: /* Publish -> is this file already published?*/
        if (status == Mantid::API::LOCAL_ONLY
            ||
            status == Mantid::API::LOCAL_CHANGED)
          return "false";
        else{
          /* If file is only remote, tis question does not apply*/
          if (status == Mantid::API::REMOTE_ONLY)
            return "";
          return "true";
        }
        break;
      case 4:
        return path;
      default:
        break;
      }
      return "";
     
    }
    break;
  case Qt::DecorationRole:
    {
      if (column > 0)
        return QVariant(); 

      if (directory){
        if (status == Mantid::API::REMOTE_ONLY)
          return QIcon::fromTheme("folder-remote", QIcon(QPixmap(":/win/folder-remote"))); 
        else
          return QIcon::fromTheme("folder", QIcon(QPixmap(":/win/folder")));           
      }
      else{
        int pos = QString(path).lastIndexOf('.');
        if (pos < 0)
          return QIcon::fromTheme("unknown", QIcon(QPixmap(":/win/unknown"))); 
        if (path.contains("readme",Qt::CaseInsensitive))
          return QIcon::fromTheme("text-x-readme", QIcon(QPixmap(":/win/txt_file.png"))); 


        QString extension = QString(path).remove(0,pos);
        if (extension == ".cpp" || extension == ".CPP" || extension == ".c" || extension == ".C")
          return QIcon::fromTheme("text-x-c++", QIcon(QPixmap(":/win/unknown")));
        else if (extension == ".py" || extension == ".PY")
          return QIcon::fromTheme("text-x-python", QIcon(QPixmap(":/win/text-x-python"))); 
        else if (extension == ".ui")
          return QIcon::fromTheme("document", QIcon(QPixmap(":/win/document")));
        else if (extension == ".docx" || extension == ".doc" || extension == ".odf")
          return QIcon::fromTheme("x-office-document", QIcon(QPixmap(":/win/office-document"))); 
        else if (extension == ".pdf")
          return QIcon::fromTheme("application-pdf", QIcon(QPixmap(":/win/file_pdf"))); 
        else
          return QIcon::fromTheme("unknown", QIcon(QPixmap(":/win/unknown"))); 

      }
      
    }
    break;
  }
  return QVariant();
}

void RepoItem::statusUpdate(Mantid::API::ScriptRepository_sptr repo){
  status = repo->fileStatus(systemPath().toStdString());
  foreach(RepoItem * child, childItems){
    child->statusUpdate(repo); 
  }
}

bool RepoItem::setData(int column , QString value,Mantid::API::ScriptRepository_sptr repo ){
  switch(column){
  case 0:    
    break;
  case 1:
  case 2: 
    if (value == "true"){
      std::string file = systemPath().toStdString(); 
      // FIXME: protect from exception
      repo->download(file); 
      
      repo->listFiles(); 

      statusUpdate(repo); 
      }    
    return true;
    break;
  case 3: 
    if (value == "true")
      status = Mantid::API::BOTH_UNCHANGED; /* simulate upload*/ 
    return true; 
    break;
  default: 
    break;
  }
  return false; 
}
    
int RepoItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<RepoItem*>(this));
    return 0;
}

RepoItem *RepoItem::parent()
{
    return parentItem;
}

QString RepoItem::systemPath()
{
  return path;
}
//////////////////////////////////////////////////
// DELEGATE
///////////////////////////////////////////////////
RepoDelegate::RepoDelegate(QObject *parent)
    :QStyledItemDelegate(parent)
{}

QWidget * RepoDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
                                     const QModelIndex &/*index*/) const
{
  QPushButton *pb = new QPushButton(parent);
  return pb;
}


void RepoDelegate::setEditorData(QWidget */*editor*/, const QModelIndex &/*index*/) const
{
  /*    QString value = index.model()->data(index, Qt::DisplayRole).toString();  // Qt::EditRole
    QPushButton * pb = static_cast<QPushButton*>(editor);
    pb->setHidden(value == "true");*/
}

void RepoDelegate::setModelData(QWidget */*editor*/, QAbstractItemModel *model,
                                const QModelIndex &index) const{  
   model->setData(index, QString("true"), Qt::EditRole);
}

void RepoDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}

void RepoDelegate::paint(
       QPainter* painter,
       const QStyleOptionViewItem & option,
       const QModelIndex & index
       ) const
{
  
  if (!index.isValid())
    return;
  if (painter->device() ==0)
    return;

  QIcon icon ; 
  QString checked = index.model()->data(index, Qt::DisplayRole).toString();
  if (index.column() == 1){
    // download    
    if (checked == "true" || checked.isEmpty())
      return;
    icon = QIcon::fromTheme("system-software-install", QIcon(QPixmap(":/win/download"))); 
  }else if (index.column() == 2){
    // update
    if (checked.isEmpty())
      return; 
    if (checked == "true")
      icon = QIcon::fromTheme("dialog-ok", QIcon(QPixmap(":/win/dialog-ok"))); 
    else
      icon = QIcon::fromTheme("bottom", QIcon(QPixmap(":win/system-software-update"))); 
  }else if (index.column() == 3){
    if (checked.isEmpty() || checked == "true")
      return;
    else
      icon = QIcon::fromTheme("add-files-to-archive", QIcon(QPixmap(":win/upload"))); 
  }

  QRect buttonRect( option.rect);
  
  int min_val = buttonRect.width()<buttonRect.height() ? buttonRect.width() : buttonRect.height();

  buttonRect.setWidth(min_val); 
  buttonRect.setHeight(min_val); 
  buttonRect.moveCenter(option.rect.center());

  QStyleOptionButton button;
  button.rect = buttonRect;
  button.icon = icon;
  int icon_size =(int) (min_val*.8); 
  button.iconSize = QSize(icon_size,icon_size);
  button.state =  QStyle::State_Enabled;
  
  QApplication::style()->drawControl
    (QStyle::CE_PushButton, &button, painter);
}

bool  RepoDelegate::editorEvent(QEvent *event,
                                   QAbstractItemModel *model,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) {
  if ((event->type() == QEvent::MouseButtonRelease) ||
      (event->type() == QEvent::MouseButtonDblClick)) {
    QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);
    if (mouse_event->button() != Qt::LeftButton ||
        !option.rect.contains(mouse_event->pos())) {
      return false;
    }
    if (event->type() == QEvent::MouseButtonDblClick) {
      return true;
    }
  } else if (event->type() == QEvent::KeyPress) {
    if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space &&
        static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select) {
      return false;
    }
  } else {
    return false;
  }

  QString checked = index.model()->data(index, Qt::DisplayRole).toString();
  return model->setData(index, checked != "true", Qt::EditRole);
}
QSize RepoDelegate::sizeHint(const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/ ) const{
  return QSize(35,35);

} ;

//////////////////////////////////////////////////
// MODEL
///////////////////////////////////////////////////

RepoModel::RepoModel(QObject *parent):
  QAbstractItemModel(parent)  
{
  ConfigServiceImpl & config = ConfigService::Instance();
  repo_path = QString::fromStdString(config.getString("ScriptLocalRepository")); 
   rootItem = new RepoItem("Root","/");
   setupModelData(rootItem);
}

RepoModel::~RepoModel()
{
     delete rootItem;
}

QVariant RepoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    RepoItem *item = static_cast<RepoItem*>(index.internalPointer());
    return item->data(index.column(), role);
}

bool RepoModel::setData(const QModelIndex & index, const QVariant & value, 
                        int role){
  if (!index.isValid())
    return false; 
  if (role != Qt::EditRole)
    return false; 
  

  RepoItem * item = static_cast<RepoItem*>(index.internalPointer());
  qDebug() << "Try to set data " << item->systemPath() << " " << value.toString() <<  endl; 
  bool ret = false;   
  ret = item->setData(index.column(),value.toString(), repo_ptr);
  if (ret){
    emit dataChanged(index, index);  
  }
  
  return ret;
}

Qt::ItemFlags RepoModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return 0 ;
  if (index.column() == 0)
    return  QAbstractItemModel::flags(index); 
  return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

QVariant RepoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
      return rootItem->data(section);
    return QVariant();
}

QModelIndex RepoModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    RepoItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<RepoItem*>(parent.internalPointer());

    RepoItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex RepoModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    RepoItem *childItem = static_cast<RepoItem*>(index.internalPointer());
    RepoItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int RepoModel::rowCount(const QModelIndex &parent) const
{
    RepoItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<RepoItem*>(parent.internalPointer());

    return parentItem->childCount();
}


int RepoModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<RepoItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

void RepoModel::entrySelected(const QModelIndex& index){
  qDebug() << "Entry selected index.row" << index.row() << endl; 
   RepoItem * item = static_cast<RepoItem*>(index.internalPointer());
   if (!item){
     qWarning() << "Entry selected is not a RepoItem\n"; 
     return;}
   QString path = item->systemPath();
  
   if (path != last_selected){
     QString info; 
     try{

        info = QString::fromStdString(repo_ptr->fileInfo(path.toStdString()).description);
     }catch(Mantid::API::ScriptRepoException & ex){
       qDebug() << "Exception : " << ex.what() << endl; 
       //ignore exception
     }
     emit fileDescription(info); /* simulate description*/ 
     last_selected = path; 
   }
   
}

void RepoModel::fileSelected(const QModelIndex& index){
  qDebug() << "File selected " << index.row() << endl; 
  if (index.column() == 0){
   RepoItem * item = static_cast<RepoItem*>(index.internalPointer());
   if (item->state() == Mantid::API::REMOTE_ONLY 
       || 
       item->isDir())
     return; // do not open remote file, just local
   
   QString path = repo_path + "/" + item->systemPath(); 
   qDebug() << "trying to emit load Script : " <<path << "\n";
   emit loadScript(path); 
  }
}

void RepoModel::reLoad(void){
  repo_ptr->listFiles(); 
  
}


void RepoModel::setupModelData(RepoItem *parent)
{
  using Mantid::API::ScriptRepositoryFactory; 
  using Mantid::API::ScriptRepository_sptr;
  using Mantid::API::ScriptRepository;
  repo_ptr = ScriptRepositoryFactory::Instance().create("GitScriptRepository");
  QStringList lines;
  repo_ptr->listFiles();
  const std::vector<ScriptRepository::file_entry> & entries = repo_ptr->listEntries(); 
  QList<RepoItem*> parents;

  parents << parent;

  QString last_directory = "";
  
  unsigned int number = 0;
  bool skip = false;

  while (number < entries.size()) {
    QString lineData = QString::fromStdString(entries[number].path);
    
    if (!lineData.isEmpty()) {
      // Read the column data from the rest of the line.
      QStringList pathStrings = lineData.split("/");
      QString current_file = "";
      for(int i = 0; i< pathStrings.size(); i++ ){
        if (pathStrings[i].isEmpty())
          continue;
        
        if (!skip){
          if (i ==0)
            current_file = pathStrings[i];
          else
            current_file.append("/").append(pathStrings[i]);
        }
        // qDebug() << "loop current_file = " << current_file << " status = " << (int) entries[number].status<<endl;
        if (current_file.startsWith(last_directory) || last_directory == "")
          {
            if (entries[number].directory){
              QStringList columnData; 
              columnData << pathStrings[i] << current_file << "nothing" << "again"; 
              parents.last()->appendChild(new RepoItem(pathStrings[i], current_file, true,
                                                       entries[number].status,
                                                       parents.last()));
              parents << parents.last()->child(parents.last()->childCount()-1);
              last_directory = current_file;
              last_directory.append("/");
            }else{
              parents.last()->appendChild(new RepoItem(pathStrings[i], 
                                                       current_file,
                                                       entries[number].directory,
                                                       entries[number].status,
                                                       parents.last()));
            }
            skip = false;
          }
        else if (last_directory.startsWith(current_file))
          {    continue; skip = false;
          }else{
          parents.pop_back();
          if (parents.last() == parent){
            last_directory = "";
          }else
            last_directory = parents.last()->systemPath().append("/");
          //qDebug() << "PopBack = "<< last_directory << endl;
          i--;
          skip = true;
        }
      }
      
    }

    number++;
    
  }
}
