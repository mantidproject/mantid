#ifndef MANTID_API_SCRIPTREPOSITORYVIEW_H_
#define MANTID_API_SCRIPTREPOSITORYVIEW_H_

#include <QDialog>
#include <QStyledItemDelegate>
#include "ui_ScriptRepositoryView.h"
#include "DllOption.h"

namespace MantidQt
{
namespace API
{
  class RepoModel;
  /** ScriptRepositoryView : Provide the User Interface to the ScriptRepository. It does so 
      through the Mantid Model View Framework. It is composed by a specialized QTreeView 
      (RepoTreeView) and a TextBrowser. The TreeView is populated with the RepoModel, wich wrappers
      the ScriptRepository. Inside this class, there are two nested classes that will implement 
      delegates to the columns of Status and AutoUpdate in order to improve the User Experience. 
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class  EXPORT_OPT_MANTIDQT_API ScriptRepositoryView : public QDialog
  {
    Q_OBJECT

      /// Delegate to show the icons Download and Upload
    class RepoDelegate : public QStyledItemDelegate
    {
    public:
      RepoDelegate(QObject *parent = 0);
      
      void paint(QPainter *painter,
                 const QStyleOptionViewItem &option,
                 const QModelIndex &index) const;
      bool editorEvent(QEvent *event,
                       QAbstractItemModel *model,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index);
      QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const ;
    };
    /// Delegate to show the checkbox for configuring the auto update
    class CheckBoxDelegate : public QStyledItemDelegate
    {
    public:
      CheckBoxDelegate(QObject * parent = 0); 
      void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
      bool editorEvent(QEvent *event,
                       QAbstractItemModel *model,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index);
    };
    /// Delegate to show the icon to remove the entry from the local and central repository
    class RemoveEntryDelegate : public QStyledItemDelegate
    {
    public:
      RemoveEntryDelegate(QObject * parent = 0); 
      void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
      bool editorEvent(QEvent *event,
                       QAbstractItemModel *model,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index);
    };

   public:
    // constuctor 
    ScriptRepositoryView(QWidget * parent=0);
    // destructor - not virtual, because this is not intended to be base
    ~ScriptRepositoryView();

  signals:
    // allow Mantid Plot to open a python file to be seen 
    void loadScript(const QString);
    
  protected slots:
    // allow to interact with the cells, in order to update the description of the files
    void cell_activated(const QModelIndex & ); 
    void updateModel();
    void currentChanged ( const QModelIndex & current );
    void helpClicked();
    void openFolderLink(QString);


  private:
    Ui::ScriptRepositoryView * ui; 
    RepoModel * model;
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_SCRIPTREPOSITORYVIEW_H_ */
