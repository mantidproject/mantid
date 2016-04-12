#include "MantidQtMantidWidgets/MuonFitDataPresenter.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param view :: [input] Pointer to view
 */
MuonFitDataPresenter::MuonFitDataPresenter(IMuonFitDataView *view)
    : m_view(view) {
  if (!m_view) {
    throw std::invalid_argument("MuonFitDataPresenter created with null view!");
  }
}

/**
 * Sets number of periods and updates checkboxes on UI
 * @param numPeriods :: [input] Number of periods in data
 */
void MuonFitDataPresenter::setNumPeriods(size_t numPeriods) {
  m_numPeriods = numPeriods;
  // TODO: UI code here, add/remove checkboxes, hide if = 0 (in presenter)
  throw std::runtime_error("TODO: implement this function");
}

/**
 * Returns a list of the selected periods
 * These are either single numbers or a combination string
 * e.g. "1", "2", "1+2-3+4"
 * @returns :: String list of period numbers / combinations
 */
QStringList MuonFitDataPresenter::getChosenPeriods() const {
  throw std::runtime_error("TODO: implement this function");
}

/**
 * Sets group names and updates checkboxes on UI
 * @param groups :: [input] List of group names
 */
void MuonFitDataPresenter::setAvailableGroups(const QStringList &groups) {
  // TODO: something here inc. UI code with checkboxes
  // Set them all checked by default
  // m_ui.verticalLayoutGroups->count();
  Q_UNUSED(groups)
  throw std::runtime_error("TODO: implement this function");
}

/**
 * Returns a list of the selected groups (checked boxes)
 * @returns :: list of selected groups
 */
QStringList MuonFitDataPresenter::getChosenGroups() const {
  // TODO: implement this function
  throw std::runtime_error("TODO: implement this function");
}

} // namespace MantidWidgets
} // namespace MantidQt
