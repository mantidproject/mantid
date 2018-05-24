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
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
          locationsOfRowsToRemove);
  void copyRowsRequested();
  void pasteRowsRequested();
  void cutRowsRequested();
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_JOBTREEVIEWSIGNALADAPTER_H_
