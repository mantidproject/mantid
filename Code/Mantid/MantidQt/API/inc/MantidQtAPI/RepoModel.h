#ifndef MANTID_API_REPOMODEL_H_
#define MANTID_API_REPOMODEL_H_
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QList>
#include <QVariant>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QWidget>
#include "MantidAPI/ScriptRepository.h"

namespace MantidQt
{
namespace API
{

  class RepoItem
  {
public:
    RepoItem(const QString label, const QString path, 
             bool entry_directory = false,
             Mantid::API::SCRIPTSTATUS curr_status = Mantid::API::BOTH_UNCHANGED, 
             RepoItem * parent = 0);

    ~RepoItem();
    void appendChild(RepoItem * child);
    RepoItem * child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column,  int role = Qt::DisplayRole  ) const;
    bool setData(int column, QString value, Mantid::API::ScriptRepository_sptr   ); 
    int row() const;
    RepoItem * parent();
    QString systemPath();
    Mantid::API::SCRIPTSTATUS state(){return status;};
    bool isDir(){return directory;}; 
    
private:
    QList<RepoItem * >childItems;
    void statusUpdate(Mantid::API::ScriptRepository_sptr);
    QString label;
    QString path; 
    Mantid::API::SCRIPTSTATUS status; 
    bool directory; 
    bool root; 
    RepoItem* parentItem;
};


class RepoDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    RepoDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const;
    bool editorEvent(QEvent *event,
                                       QAbstractItemModel *model,
                                       const QStyleOptionViewItem &option,
                                       const QModelIndex &index);
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const ;
};


  /** RepoModel : TODO: DESCRIPTION
    
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
public:
    RepoModel(QObject *parent = 0);
    ~RepoModel();

    QVariant data(const QModelIndex & index, int role)const;
    Qt::ItemFlags flags(const QModelIndex & index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool setData(const QModelIndex &index, const QVariant &value,
                  int role = Qt::EditRole);
    public slots:
    void entrySelected(const QModelIndex& index); 
    void fileSelected(const QModelIndex & index); 
    void reLoad();
 signals: 
    void fileDescription(const QString ); 
    void loadScript(const QString); 
private:
    void setupModelData(RepoItem *parent);

    RepoItem *rootItem;
    Mantid::API::ScriptRepository_sptr repo_ptr;
    QString last_selected;
    QString repo_path;
  };


}; // namespace API
};// namespace Mantid

#endif  /* MANTID_API_SCRIPTREPOSITORYVIEW_H_ */
