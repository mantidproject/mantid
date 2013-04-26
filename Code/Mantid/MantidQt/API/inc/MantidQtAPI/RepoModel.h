#ifndef MANTID_API_REPOMODEL_H_
#define MANTID_API_REPOMODEL_H_
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QVariant>
#include <QStringList>
#include <QWidget>
#include <QDialog>
#include "MantidAPI/ScriptRepository.h"
#include <QtConcurrentRun>
#include <QFutureWatcher>

class QLineEdit; 
class QCheckBox; 
class QTextEdit;

namespace Mantid{
  namespace Kernel{
    class Logger;
  }
}
namespace MantidQt
{
namespace API
{

const QString REMOTEONLY = "REMOTE_ONLY"; 
const QString LOCALONLY = "LOCAL_ONLY";
const QString LOCALCHANGED = "LOCAL_CHANGED";
const QString REMOTECHANGED = "REMOTE_CHANGED";
const QString BOTHUNCHANGED = "UPDATED";
const QString BOTHCHANGED = "CHANGED"; 
const QString UPLOADST = "UPLOADING";
const QString DOWNLOADST = "DOWNLOADING";

  /** RepoModel : Wrapper for ScriptRepository to fit the Model View Qt Framework. 
      
      The ScriptRepository has an hierarchical access to the folders and files, as so, 
      it was necessary to extend the QAbstractItemModel in order to provide access to the 
      entries on ScriptRepository.

      The RepoModel will be given to a QTreeView and as so, it will allow the user to interact 
      with the ScriptRepository through the Qt ModelView Framework. 

      The requirements for a class to fit this framework is to reimplement the following methods: 
        - ::data - giving access to the data
        - ::flags - indication of the allowed interaction 
        - ::headerData 
        - ::index
        - ::parent
        - ::rowCount
        - ::columnCount
        - ::setData

     Through these methods, the RepoModel will be able to provide access to the ScriptRepository 
     service. Allowing the users to upload and download files and folders.

     Some extra services are provided, to allow the classes to show the description of the files 
     as well as open the files to be inspected by the users: 
      - ::entrySelected
      - ::fileSelected

     This class should be constructed just once, and as so, the copy constructor and the assignment 
     will be make private to ensure this. 
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class RepoModel : public QAbstractItemModel
{
    Q_OBJECT

  /** A nested class to help RepoModel to implement the QAbstractItemModel. 
      This class in indended to keep track of the path inside the Repository
      of the entries, in such a way the repomodel will be able to get from the 
      index the path.
      The ::appendChild allows the RepoModel to reconstruct the tree of the directories,
      while the ::child and ::parent methods allow to iterate over the tree.      
  */
  class RepoItem
  {
public:
    // construct the RepoItem passing the script repository path
    RepoItem(const QString & label, 
             const QString & path = "/",
             RepoItem * parent = 0);

    ~RepoItem();
    // append child to build the directory tree
    void appendChild(RepoItem * child);
    // access to the row_th file/folder child of this entry
    RepoItem * child(int row) const;
    /// To which row this repoItem belongs?
    int row() const;
    // return the number of files/folders that are children of this entry
    int childCount() const;
    /// access to the script repository path
    /// @return : script repository path
    const QString & path() const{return keypath;}; 
    /// access to the label provided at construction
    /// @return : label for this entry
    const QString & label()const{return m_label;}; 
    /// access to the parent of this entry
    /// @return : this entry parent's
    RepoItem * parent() const{return parentItem;};
    
private:
    /// track the list of children for this entry
    QList<RepoItem * >childItems;
    /// the label of this entry
    QString m_label; 
    /// the path of the script repository
    QString keypath; 
    /// the parent of this entry
    RepoItem* parentItem;
private:
    RepoItem( const RepoItem& );
    const RepoItem& operator=( const RepoItem& );
};

    class UploadForm: public QDialog{
    public:
      UploadForm(const QString & file2upload, QWidget * parent = 0);
      virtual ~UploadForm();
      QString email(); 
      QString author(); 
      QString comment();
      bool saveInfo();
      void setEmail(const QString& ); 
      void setAuthor(const QString&);
      void lastSaveOption(bool option);

    protected: 
      QLineEdit * author_le; 
      QLineEdit * email_le; 
      QCheckBox * save_ck;
      QTextEdit * comment_te;
    };


public:
    /// constructor
    RepoModel(QObject *parent = 0);
    /// destructor
    ~RepoModel();
    /// access to the ScriptRepository data
    QVariant data(const QModelIndex & index, int role)const;
    /// information on the available interaction
    Qt::ItemFlags flags(const QModelIndex & index) const;
    /// header strings
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    /// access to the index
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    /// access to parent
    QModelIndex parent(const QModelIndex &index) const;
    //// provide the number of the rows
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    /// provide the nubmer of the coluns
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    /// change data
    bool setData(const QModelIndex &index, const QVariant &value,
                  int role = Qt::EditRole);

    static const QString & localOnlySt(); 
    static const QString & remoteOnlySt(); 
    static const QString & localChangedSt(); 
    static const QString & remoteChangedSt(); 
    static const QString & updatedSt(); 
    static const QString & bothChangedSt(); 
    static const QString & downloadSt();
    static const QString & uploadSt();

    QString fileDescription(const QModelIndex & index); 
    QString filePath(const QModelIndex & index);
    QString author(const QModelIndex& index); 
    
private:
    /// auxiliary method to populate the model
    void setupModelData(RepoItem *parent);
    /// auxiliary method to match the ScriptStatus to string
    const QString & fromStatus(Mantid::API::SCRIPTSTATUS status)const; 

    RepoItem *rootItem;
    Mantid::API::ScriptRepository_sptr repo_ptr;
    QString last_selected;
    QString repo_path;
    /// auxiliary method to help populating the model
    RepoItem * getParent(const QString & folder, QList<RepoItem*>&parents);
private:
    RepoModel( const RepoModel& );
    const RepoModel& operator=( const RepoModel& );
    static Mantid::Kernel::Logger & g_log;
    void handleExceptions(const Mantid::API::ScriptRepoException & ex, 
                          const QString & title, 
                          bool showWarning=true)const;

    //handle download in thread
    QFuture<QString> download_threads;
    QFutureWatcher<QString> download_watcher;
    QModelIndex download_index;
    QString downloading_path;
    bool isDownloading(const QModelIndex & index)const ; 
    private slots:
    void downloadFinished();

 private:
    //handle upload in thread
    QFuture<QString> upload_threads;
    QFutureWatcher<QString> upload_watcher;
    QModelIndex upload_index;
    QString uploading_path;
    bool isUploading(const QModelIndex & index)const ; 
    private slots:
    void uploadFinished();

  };


}; // namespace API
};// namespace Mantid

#endif  /* MANTID_API_SCRIPTREPOSITORYVIEW_H_ */
