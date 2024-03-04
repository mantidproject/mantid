// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtWidgets/Common/WorkspaceMultiSelector.h"

#include <Poco/AutoPtr.h>
#include <Poco/NObserver.h>
#include <Poco/Notification.h>
#include <Poco/NotificationCenter.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QCompleter>
#include <QDebug>
#include <QHeaderView>
#include <QLineEdit>
#include <QStyledItemDelegate>

constexpr short namesCol = 0;
constexpr short indexCol = 1;

namespace {
using namespace Mantid::API;

QStringList headerLabels = {"Workspace Name", "Ws Index"};

MatrixWorkspace_sptr getWorkspace(const std::string &name) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
}

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

bool validWorkspace(std::string const &name) { return !name.empty() && doesExistInADS(name); }

boost::optional<std::size_t> maximumIndex(const MatrixWorkspace_sptr &workspace) {
  if (workspace) {
    const auto numberOfHistograms = workspace->getNumberHistograms();
    if (numberOfHistograms > 0)
      return numberOfHistograms - 1;
  }
  return boost::none;
}

QString getIndexString(const MatrixWorkspace_sptr &workspace) {
  const auto maximum = maximumIndex(workspace);
  if (maximum) {
    if (*maximum > 0)
      return QString("0-%1").arg(*maximum);
    return "0";
  }
  return "";
}

QString getIndexString(const std::string &workspaceName) { return getIndexString(getWorkspace(workspaceName)); }

std::unique_ptr<QRegExpValidator> createValidator(const QString &regex, QObject *parent) {
  return std::make_unique<QRegExpValidator>(QRegExp(regex), parent);
}

QString OR(const QString &lhs, const QString &rhs) { return "(" + lhs + "|" + rhs + ")"; }

QString NATURAL_NUMBER(std::size_t digits) { return OR("0", "[1-9][0-9]{," + QString::number(digits - 1) + "}"); }

const QString EMPTY = "^$";
const QString SPACE = "(\\s)*";
const QString COMMA = SPACE + "," + SPACE;
const QString MINUS = "\\-";

const QString NUMBER = NATURAL_NUMBER(4);
const QString NATURAL_RANGE = "(" + NUMBER + MINUS + NUMBER + ")";
const QString NATURAL_OR_RANGE = OR(NATURAL_RANGE, NUMBER);
const QString SPECTRA_LIST = "(" + NATURAL_OR_RANGE + "(" + COMMA + NATURAL_OR_RANGE + ")*)";

class SpectraListDelegate : public QStyledItemDelegate {
public:
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                        const QModelIndex & /*index*/) const override {
    auto lineEdit = std::make_unique<QLineEdit>(parent);
    auto validator = createValidator(SPECTRA_LIST, parent);
    lineEdit->setValidator(validator.release());
    return lineEdit.release();
  }

  void setEditorData(QWidget *editor, const QModelIndex &index) const override {
    const auto value = index.model()->data(index, Qt::EditRole).toString();
    static_cast<QLineEdit *>(editor)->setText(value);
  }

  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
    auto *lineEdit = static_cast<QLineEdit *>(editor);
    model->setData(index, lineEdit->text(), Qt::EditRole);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex & /*index*/) const override {
    editor->setGeometry(option.rect);
  }
};
} // namespace

/**
 * Default constructor
 * @param parent :: A widget to act as this widget's parent (default = NULL)
 * @param init :: If true then the widget will make calls to the framework
 * (default = true)
 */
namespace MantidQt {
namespace MantidWidgets {

WorkspaceMultiSelector::WorkspaceMultiSelector(QWidget *parent, bool init)
    : QTableWidget(parent), m_addObserver(*this, &WorkspaceMultiSelector::handleAddEvent),
      m_remObserver(*this, &WorkspaceMultiSelector::handleRemEvent),
      m_clearObserver(*this, &WorkspaceMultiSelector::handleClearEvent),
      m_renameObserver(*this, &WorkspaceMultiSelector::handleRenameEvent),
      m_replaceObserver(*this, &WorkspaceMultiSelector::handleReplaceEvent), m_init(init), m_connected(false),
      m_workspaceTypes(), m_showHidden(false), m_showGroups(true), m_binLimits(std::make_pair(0, -1)), m_suffix() {

  if (init) {
    connectObservers();
  }
}

/**
 * Destructor for WorkspaceMultiSelector
 * De-subscribes this object from the Poco NotificationCentre
 */
WorkspaceMultiSelector::~WorkspaceMultiSelector() { disconnectObservers(); }

/**
 * Setup table dimensions and headers from the parent class.
 */
void WorkspaceMultiSelector::setupTable() {
  this->setRowCount(0);
  this->setColumnCount(headerLabels.size());
  this->verticalHeader()->setVisible(false);
  this->horizontalHeader()->setVisible(true);
  this->setHorizontalHeaderLabels(headerLabels);
  this->setItemDelegateForColumn(1, new SpectraListDelegate);
  this->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->setSortingEnabled(true);
  this->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}
/**
 * De-subscribes this object from the Poco NotificationCentre
 */
void WorkspaceMultiSelector::disconnectObservers() {
  if (m_init) {
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_addObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_remObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_clearObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_renameObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
    m_init = false;
    m_connected = false;
  }
}

/**
 * Subscribes this object to the Poco NotificationCentre
 */
void WorkspaceMultiSelector::connectObservers() {
  Mantid::API::AnalysisDataServiceImpl &ads = Mantid::API::AnalysisDataService::Instance();
  ads.notificationCenter.addObserver(m_addObserver);
  ads.notificationCenter.addObserver(m_remObserver);
  ads.notificationCenter.addObserver(m_renameObserver);
  ads.notificationCenter.addObserver(m_clearObserver);
  ads.notificationCenter.addObserver(m_replaceObserver);
  refresh();
  m_init = true;
  m_connected = true;
}

QStringList WorkspaceMultiSelector::getWorkspaceTypes() const { return m_workspaceTypes; }

void WorkspaceMultiSelector::setWorkspaceTypes(const QStringList &types) {
  if (types != m_workspaceTypes) {
    m_workspaceTypes = types;
    if (m_init) {
      refresh();
    }
  }
}

bool WorkspaceMultiSelector::showHiddenWorkspaces() const { return m_showHidden; }

void WorkspaceMultiSelector::showHiddenWorkspaces(bool show) {
  if (show != m_showHidden) {
    m_showHidden = show;
    if (m_init) {
      refresh();
    }
  }
}

bool WorkspaceMultiSelector::showWorkspaceGroups() const { return m_showGroups; }

void WorkspaceMultiSelector::showWorkspaceGroups(bool show) {
  if (show != m_showGroups) {
    m_showGroups = show;
    if (m_init) {
      refresh();
    }
  }
}

bool WorkspaceMultiSelector::isValid() const {
  QTableWidgetItem *item = currentItem();
  return (item->text() != "");
}

bool WorkspaceMultiSelector::isConnected() const { return m_connected; }

QStringList WorkspaceMultiSelector::getWSSuffixes() const { return m_suffix; }

void WorkspaceMultiSelector::setWSSuffixes(const QStringList &suffix) {
  if (suffix != m_suffix) {
    m_suffix = suffix;
    if (m_init) {
      refresh();
    }
  }
}

void WorkspaceMultiSelector::setLowerBinLimit(int numberOfBins) { m_binLimits.first = numberOfBins; }

void WorkspaceMultiSelector::setUpperBinLimit(int numberOfBins) { m_binLimits.second = numberOfBins; }

void WorkspaceMultiSelector::addItem(const std::string &name) {
  insertRow(rowCount());
  auto nameItem = std::make_unique<QTableWidgetItem>(QString::fromStdString(name));
  auto indexItem = std::make_unique<QTableWidgetItem>(getIndexString(name));

  nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

  setItem(rowCount() - 1, namesCol, nameItem.release());
  setItem(rowCount() - 1, indexCol, indexItem.release());
  return;
}

void WorkspaceMultiSelector::renameItem(const std::string &newName, int row) {
  // here is assuming item has already been deemed eligible
  this->item(row, namesCol)->setText(QString::fromStdString(newName));
  this->item(row, indexCol)->setText(getIndexString(newName));
}

void WorkspaceMultiSelector::addItems(const std::vector<std::string> &names) {
  auto nItems = names.size();
  for (auto const &name : names) {
    if (checkEligibility(name)) {
      addItem(name);
    }
  }
  return;
}

stringPairVec WorkspaceMultiSelector::retrieveSelectedNameIndexPairs() {

  auto selIndexes = selectedIndexes();
  stringPairVec nameIndexPairVec;
  nameIndexPairVec.reserve(static_cast<std::size_t>(selIndexes.size()));

  for (auto const &index : selIndexes) {
    std::string txt = item(index.row(), namesCol)->text().toStdString();
    if (txt != "") {
      std::string idx = item(index.row(), indexCol)->text().toStdString();
      nameIndexPairVec.push_back(std::make_pair(txt, idx));
    }
  }
  nameIndexPairVec.shrink_to_fit();
  return nameIndexPairVec;
}

void WorkspaceMultiSelector::resetIndexRangeToDefault() {
  auto selIndex = this->selectedIndexes();
  if (!selIndex.isEmpty()) {
    for (auto &index : selIndex) {
      std::string selName = this->item(index.row(), namesCol)->text().toStdString();
      this->item(index.row(), indexCol)->setText(getIndexString(selName));
    }
  }
}

void WorkspaceMultiSelector::unifyRange() {
  auto selIndex = this->selectedIndexes();
  if (!selIndex.isEmpty()) {
    auto rangeFirst = this->item(selIndex.takeFirst().row(), indexCol)->text();
    for (auto &index : selIndex) {
      this->item(index.row(), indexCol)->setText(rangeFirst);
    }
  }
}

void WorkspaceMultiSelector::handleAddEvent(Mantid::API::WorkspaceAddNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  if (!showHiddenWorkspaces() &&
      Mantid::API::AnalysisDataService::Instance().isHiddenDataServiceObject(pNf->objectName())) {
    return;
  }
  if (checkEligibility(pNf->objectName())) {
    addItem(pNf->objectName());
  }
}

void WorkspaceMultiSelector::handleRemEvent(Mantid::API::WorkspacePostDeleteNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  QString name = QString::fromStdString(pNf->objectName());
  auto items = findItems(name, Qt::MatchExactly);
  if (!items.isEmpty()) {
    for (auto &item : items)
      removeRow(item->row());
  }
  if (rowCount() == 0) {
    emit emptied();
  }
}

void WorkspaceMultiSelector::handleClearEvent(Mantid::API::ClearADSNotification_ptr) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  this->clearContents();
  while (rowCount() > 0) {
    removeRow(0);
  }
  emit emptied();
}

void WorkspaceMultiSelector::handleRenameEvent(Mantid::API::WorkspaceRenameNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);

  QString newName = QString::fromStdString(pNf->newObjectName());
  QString currName = QString::fromStdString(pNf->objectName());

  bool eligible = checkEligibility(pNf->newObjectName());
  auto currItems = findItems(currName, Qt::MatchExactly);
  auto newItems = findItems(newName, Qt::MatchExactly);

  if (eligible) {
    if (!currItems.isEmpty() && newItems.isEmpty())
      renameItem(pNf->newObjectName(), currItems.first()->row());
    else if (currItems.isEmpty() && newItems.isEmpty())
      addItem(pNf->newObjectName());
    else if (!currItems.isEmpty() && !newItems.isEmpty()) {
      // list reduction w. redundancies
      removeRow(currItems.first()->row());
      renameItem(pNf->newObjectName(), newItems.first()->row());
    }
  } else {
    if (!currItems.isEmpty())
      removeRow(currItems.first()->row());
  }
}

void WorkspaceMultiSelector::handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  QString name = QString::fromStdString(pNf->objectName());
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  bool eligible = checkEligibility(pNf->objectName());
  auto items = findItems(name, Qt::MatchExactly);

  if ((eligible && !items.isEmpty()) || (!eligible && items.isEmpty()))
    return;
  else if (items.isEmpty() && eligible) {
    addItem(pNf->objectName());
  } else { // (inside && !eligible)
    removeRow(items.first()->row());
  }
}

bool WorkspaceMultiSelector::checkEligibility(const std::string &name) const {
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  auto workspace = ads.retrieve(name);
  if ((!m_workspaceTypes.empty()) && m_workspaceTypes.indexOf(QString::fromStdString(workspace->id())) == -1) {
    return false;
  } else if (!hasValidSuffix(QString::fromStdString(name))) {
    return false;
  } else if (!hasValidNumberOfBins(workspace)) {
    return false;
  } else if (!m_showGroups) {
    auto group = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(workspace);
    if (group != nullptr)
      return false;
  }

  return true;
}

bool WorkspaceMultiSelector::hasValidSuffix(const QString &name) const {
  if (m_suffix.isEmpty()) {
    return true;
  } else {
    for (int i = 0; i < m_suffix.size(); ++i) {
      if (name.endsWith(m_suffix[i])) {
        return true;
      }
    }
  }

  return false;
}

bool WorkspaceMultiSelector::hasValidNumberOfBins(const Mantid::API::Workspace_sptr &object) const {
  if (m_binLimits.first != 0 || m_binLimits.second != -1) {
    if (auto const workspace = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(object)) {
      auto const numberOfBins = static_cast<int>(workspace->y(0).size());
      if (m_binLimits.second != -1)
        return numberOfBins >= m_binLimits.first && numberOfBins <= m_binLimits.second;
      return numberOfBins >= m_binLimits.first;
    }
  }
  return true;
}

void WorkspaceMultiSelector::refresh() {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  this->clearContents();
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  std::vector<std::string> items;
  if (showHiddenWorkspaces()) {
    items = ads.getObjectNames(Mantid::Kernel::DataServiceSort::Sorted, Mantid::Kernel::DataServiceHidden::Include);
  } else {
    items = ads.getObjectNames();
  }
  addItems(items);
}

/**
 * Called when there is an interaction with the widget.
 */
void WorkspaceMultiSelector::focusInEvent(QFocusEvent *) { emit focussed(); }

} // namespace MantidWidgets
} // namespace MantidQt
