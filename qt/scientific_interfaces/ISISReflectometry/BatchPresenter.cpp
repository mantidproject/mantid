#include "BatchPresenter.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
namespace MantidQt {
namespace CustomInterfaces {

BatchPresenterFactory::BatchPresenterFactory(
    std::vector<std::string> const &instruments)
    : m_instruments(instruments) {}

std::unique_ptr<BatchPresenter> BatchPresenterFactory::operator()(IBatchView *view) const {
  return std::make_unique<BatchPresenter>(view, m_instruments);
}

BatchPresenter::BatchPresenter(IBatchView *view,
                               std::vector<std::string> const &instruments)
    : m_view(view), m_instruments(instruments) {
  m_view->subscribe(this);
}

void BatchPresenter::notifyProcessRequested() {}

void BatchPresenter::notifyPauseRequested() {}

void BatchPresenter::notifyExpandAllRequested() { m_view->expandAllGroups(); }

void BatchPresenter::notifyCellTextChanged(
    MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int column,
    std::string const &oldValue, std::string const &newValue) {}

bool BatchPresenter::isGroupLocation(
    MantidQt::MantidWidgets::Batch::RowLocation const &location) {
  return location.depth() == 1;
}

void BatchPresenter::notifyRowInserted(
    MantidQt::MantidWidgets::Batch::RowLocation const &newRowLocation) {
  if (newRowLocation.depth() > DEPTH_LIMIT)
    // m_view->removeGroup(newRowLocation.path()[0]);
    return;
  else if (isGroupLocation(newRowLocation))
    // kill the cells.
    return;
}

void BatchPresenter::notifyRemoveRowsRequested(
    std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
        locationsOfRowsToRemove) {}

void BatchPresenter::notifyCopyRowsRequested() {}

void BatchPresenter::notifyPasteRowsRequested() {}

void BatchPresenter::notifyFilterReset() {}
}
}
