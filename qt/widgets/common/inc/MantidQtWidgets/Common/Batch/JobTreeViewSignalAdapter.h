#ifndef MANTIDQTMANTIDWIDGETS_JOBTREEVIEWSIGNALADAPTER_H_
#define MANTIDQTMANTIDWIDGETS_JOBTREEVIEWSIGNALADAPTER_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Batch/JobTreeView.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
class EXPORT_OPT_MANTIDQT_COMMON JobTreeViewSignalAdapter
    : public QObject,
      public JobTreeViewSubscriber {
  Q_OBJECT
public:
  JobTreeViewSignalAdapter(JobTreeView &view, QObject *parent = nullptr);

  void notifyCellChanged(RowLocation const &itemIndex, int column,
                         std::string const &newValue) override;
  void notifyRowInserted(RowLocation const &newRowLocation) override;
  void notifyRemoveRowsRequested(
      std::vector<RowLocation> const &locationsOfRowsToRemove) override;
  void notifyCopyRowsRequested(
      std::vector<RowLocation> const &locationsOfRowsToCopy) override;
signals:
  void cellChanged(MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex,
                   int column, std::string newValue);
  void rowInserted(
      MantidQt::MantidWidgets::Batch::RowLocation const &newRowLocation);
  void removeRowsRequested(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
          locationsOfRowsToRemove);
  void copyRowsRequested(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
          locationsOfRowsToRemove);
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_JOBTREEVIEWSIGNALADAPTER_H_
