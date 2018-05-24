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
  BatchPresenter(IBatchView *view, std::vector<std::string> const &instruments);

  void notifyProcessRequested() override;
  void notifyPauseRequested() override;
  void notifyExpandAllRequested() override;
  void notifyCollapseAllRequested() override;
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
  void notifyCutRowsRequested() override;
  void notifyFilterReset() override;

  void notifyInsertRowRequested() override;
  void notifyInsertGroupRequested() override;
  void notifyDeleteRowRequested() override;
  void notifyDeleteGroupRequested() override;

private:
  bool
  isGroupLocation(MantidQt::MantidWidgets::Batch::RowLocation const &location) const;
  bool containsGroups(std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations) const;
  std::vector<int> groupIndexesFromSelection(
      std::vector<MantidWidgets::Batch::RowLocation> const &selected) const;
  static auto constexpr DEPTH_LIMIT = 2;
  IBatchView *m_view;
  std::vector<std::string> m_instruments;
  boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>> m_clipboard;
};

class MANTIDQT_ISISREFLECTOMETRY_DLL BatchPresenterFactory {
public:
  BatchPresenterFactory(std::vector<std::string> const &instruments);
  std::unique_ptr<BatchPresenter> operator()(IBatchView *view) const;

private:
  std::vector<std::string> m_instruments;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
