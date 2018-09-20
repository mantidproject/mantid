/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
#ifndef MANTIDQTMANTIDWIDGETS_JOBTREEVIEWSIGNALADAPTER_H_
#define MANTIDQTMANTIDWIDGETS_JOBTREEVIEWSIGNALADAPTER_H_
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
#include "MantidQtWidgets/Common/DllOption.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
class EXPORT_OPT_MANTIDQT_COMMON JobTreeViewSignalAdapter
    : public QObject,
      public JobTreeViewSubscriber {
  Q_OBJECT
public:
  JobTreeViewSignalAdapter(JobTreeView &view, QObject *parent = nullptr);

  void notifyCellTextChanged(RowLocation const &itemIndex, int column,
                             std::string const &oldValue,
                             std::string const &newValue) override;
  void notifyRowInserted(RowLocation const &newRowLocation) override;
  void notifyRemoveRowsRequested(
      std::vector<RowLocation> const &locationsOfRowsToRemove) override;
  void notifyCopyRowsRequested() override;
  void notifyCutRowsRequested() override;
  void notifyPasteRowsRequested() override;
  void notifyFilterReset() override;
signals:
  void
  cellTextChanged(MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex,
                  int column, std::string const &oldValue,
                  std::string const &newValue);
  void rowInserted(
      MantidQt::MantidWidgets::Batch::RowLocation const &newRowLocation);
  void filterReset();
  void removeRowsRequested(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const
          &locationsOfRowsToRemove);
  void copyRowsRequested();
  void pasteRowsRequested();
  void cutRowsRequested();
};
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_JOBTREEVIEWSIGNALADAPTER_H_
