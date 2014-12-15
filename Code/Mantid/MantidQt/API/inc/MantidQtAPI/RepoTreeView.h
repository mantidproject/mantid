#ifndef MANTID_API_REPOTREEVIEW_H_
#define MANTID_API_REPOTREEVIEW_H_

#include "DllOption.h"
#include <QTreeView>

namespace MantidQt
{
namespace API
{
  /** RepoTreeView : A specialization of QTreeView class that emits signal every time
      the selection change. It extends the currentChanged method in order to add the 
      emition of the signal currentCell.
    
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
  class  EXPORT_OPT_MANTIDQT_API RepoTreeView : public QTreeView
  {
    Q_OBJECT

   public:
    // constuctor 
    RepoTreeView(QWidget * parent=0):QTreeView(parent){};
    // destructor - not virtual, because this is not intended to be base
    virtual ~RepoTreeView(){};

  signals:
    void currentCell(const QModelIndex& ); 
  
  protected slots:

    void 	currentChanged ( const QModelIndex & current, const QModelIndex & previous ){
      QTreeView::currentChanged(current,previous); 
      emit currentCell(current);
    };
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_REPOTREEVIEW_H_ */
