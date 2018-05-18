#ifndef MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
#include "DllConfig.h"
#include <memory>
#include "IBatchView.h"

namespace MantidQt {
namespace CustomInterfaces {


class MANTIDQT_ISISREFLECTOMETRY_DLL BatchPresenter
    : public BatchViewSubscriber {
public:
  BatchPresenter(IBatchView* view, std::vector<std::string> const& instruments);

  void notifyProcessRequested() override;
  void notifyPauseRequested() override;
  void notifyExpandAllRequested() override;
  void notifyCellTextChanged(
      MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int column,
      std::string const &oldValue, std::string const &newValue) override;
  void notifyRowInserted(MantidQt::MantidWidgets::Batch::RowLocation const &
                             newRowLocation) override;
  void notifyRemoveRowsRequested(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
          locationsOfRowsToRemove) override;
  void notifyCopyRowsRequested() override;
  void notifyPasteRowsRequested() override;
  void notifyFilterReset() override;

private:
  bool
  isGroupLocation(MantidQt::MantidWidgets::Batch::RowLocation const &location);
  static auto constexpr DEPTH_LIMIT = 2;
  IBatchView* m_view;
  std::vector<std::string> m_instruments;
};

class BatchPresenterFactory {
public:
  BatchPresenterFactory(std::vector<std::string> const& instruments);
  std::unique_ptr<BatchPresenter> operator()(IBatchView* view) const;
private:
  std::vector<std::string> m_instruments;
};

}
}
#endif // MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
