#include <QAbstractTableModel>
#include <boost/shared_ptr.hpp>
#include "MantidQtCustomInterfaces/Updateable.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"

// Forward declarations
namespace Mantid
{
  namespace API
  {
    class ITableWorkspace;
  }
}

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** @class QtWorkspaceMementoModel

    QtWorkspaceMementoModel is a QAbstractTableModel wrapping a table workspace in order to serve-up
    display specific workspace memento views.

    @author Owen Arnold
    @date 28/09/2011

    Copyright &copy; 2011-12 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
    class QtWorkspaceMementoModel : public QAbstractTableModel, public Updateable
    {
    public:
      QtWorkspaceMementoModel(const WorkspaceMementoCollection& displayData);
      void update();
      int rowCount(const QModelIndex &parent) const;
      int columnCount(const QModelIndex &parent) const;
      QVariant data(const QModelIndex &index, int role) const;
      QVariant headerData(int section, Qt::Orientation orientation, int role) const;
      Qt::ItemFlags flags(const QModelIndex &index) const;
      ~QtWorkspaceMementoModel();

    private:

      /// Collection of data for viewing.
      const WorkspaceMementoCollection& m_displayData;
    };
  }
}