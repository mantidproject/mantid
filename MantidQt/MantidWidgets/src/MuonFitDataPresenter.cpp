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
 * Hide "Periods" section if single-period data
 * @param numPeriods :: [input] Number of periods in data
 */
void MuonFitDataPresenter::setNumPeriods(size_t numPeriods) {
  m_numPeriods = numPeriods;
  m_view->setNumPeriodCheckboxes(numPeriods);
  m_view->setPeriodVisibility(numPeriods > 1);
}

/**
 * Returns a list of the selected periods
 * These are either single numbers or a combination string
 * e.g. "1", "2", "1+2-3+4"
 * @returns :: String list of period numbers / combinations
 */
QStringList MuonFitDataPresenter::getChosenPeriods() const {
  return m_view->getPeriodSelections();
}

/**
 * Sets group names and updates checkboxes on UI
 * By default sets all checked
 * @param groups :: [input] List of group names
 */
void MuonFitDataPresenter::setAvailableGroups(const QStringList &groups) {
  m_view->clearGroupCheckboxes();
  for (const auto group : groups) {
    m_view->addGroupCheckbox(group);
    m_view->setGroupSelected(group, true);
    m_groups.append(group);
  }
}

/**
 * Returns a list of the selected groups (checked boxes)
 * @returns :: list of selected groups
 */
QStringList MuonFitDataPresenter::getChosenGroups() const {
  QStringList chosen;
  for (const auto group : m_groups) {
    if (m_view->isGroupSelected(group)) {
      chosen.append(group);
    }
  }
  return chosen;
}

} // namespace MantidWidgets
} // namespace MantidQt
