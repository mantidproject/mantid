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
#include <QDropEvent>
#include <QLineEdit>
#include <QMimeData>
#include <QUrl>
using namespace MantidQt::MantidWidgets;

/**
 * Default constructor
 * @param parent :: A widget to act as this widget's parent (default = NULL)
 * @param init :: If true then the widget will make calls to the framework
 * (default = true)
 */
WorkspaceMultiSelector::WorkspaceMultiSelector(QWidget *parent, bool init)
    : QListWidget(parent), m_addObserver(*this, &WorkspaceMultiSelector::handleAddEvent),
      m_remObserver(*this, &WorkspaceMultiSelector::handleRemEvent),
      m_clearObserver(*this, &WorkspaceMultiSelector::handleClearEvent),
      m_renameObserver(*this, &WorkspaceMultiSelector::handleRenameEvent),
      m_replaceObserver(*this, &WorkspaceMultiSelector::handleReplaceEvent), m_init(init), m_workspaceTypes(),
      m_showHidden(false), m_showGroups(true), m_optional(false), m_binLimits(std::make_pair(0, -1)), m_suffix(),
      m_algName(), m_algPropName(), m_algorithm() {
  if (init) {
    connectObservers();
  }
  this->setAcceptDrops(true);
  this->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->setSortingEnabled(true);
}

/**
 * Destructor for WorkspaceMultiSelector
 * De-subscribes this object from the Poco NotificationCentre
 */
WorkspaceMultiSelector::~WorkspaceMultiSelector() { disconnectObservers(); }

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
  QListWidgetItem *item = currentItem();
  return (item->text() != "");
}

bool WorkspaceMultiSelector::isOptional() const { return m_optional; }

void WorkspaceMultiSelector::setOptional(bool optional) {
  if (optional != m_optional) {
    m_optional = optional;
    if (m_init)
      refresh();
  }
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

void WorkspaceMultiSelector::handleAddEvent(Mantid::API::WorkspaceAddNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  if (!showHiddenWorkspaces() &&
      Mantid::API::AnalysisDataService::Instance().isHiddenDataServiceObject(pNf->objectName())) {
    return;
  }
  QString name = QString::fromStdString(pNf->objectName());
  if (checkEligibility(name, pNf->object())) {
    addItem(name);
  }
}

void WorkspaceMultiSelector::handleRemEvent(Mantid::API::WorkspacePostDeleteNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  QString name = QString::fromStdString(pNf->objectName());
  auto items = findItems(name, Qt::MatchExactly);
  if (!items.isEmpty()) {
    for (auto item : items)
      delete item;
  }
  if (count() <= 0) {
    emit emptied();
  }
}

void WorkspaceMultiSelector::handleClearEvent(Mantid::API::ClearADSNotification_ptr) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  this->clear();
  if (m_optional)
    addItem("");
  emit emptied();
}

void WorkspaceMultiSelector::handleRenameEvent(Mantid::API::WorkspaceRenameNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  QString name = QString::fromStdString(pNf->objectName());
  QString newName = QString::fromStdString(pNf->newObjectName());
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  // 1 rename per notification?
  bool eligible = checkEligibility(newName, ads.retrieve(pNf->newObjectName()));
  auto items = findItems(name, Qt::MatchExactly);
  std::cout << "I'm a rename event" << std::endl;
  if (eligible) {
    addItem(newName);
  }
  if (!items.isEmpty()) {
    for (auto item : items) {
      delete item;
    }
  }
}

void WorkspaceMultiSelector::handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  QString name = QString::fromStdString(pNf->objectName());
  auto &ads = Mantid::API::AnalysisDataService::Instance();

  bool eligible = checkEligibility(name, ads.retrieve(pNf->objectName()));
  auto items = findItems(name, Qt::MatchExactly);
  std::cout << "I'm a replace event " << std::endl;
  std::cout << name.toStdString() << std::endl;
  if (!items.isEmpty()) {
    std::cout << items[0]->text().toStdString() << std::endl;
  }

  // if it is inside and it is eligible do nothing
  // if it is not inside and it is eligible insert
  // if it is inside and it is not eligible remove
  // if it is not inside and it is not eligible do nothing

  if ((!items.isEmpty() && eligible) || (items.isEmpty() && !eligible))
    return;
  else if (items.isEmpty() && eligible) {
    addItem(name);
  } else { // (inside && !eligible)
    for (auto item : items) {
      delete item;
    }
  }
}

bool WorkspaceMultiSelector::checkEligibility(const QString &name, const Mantid::API::Workspace_sptr &object) const {
  if (m_algorithm && !m_algPropName.isEmpty()) {
    try {
      m_algorithm->setPropertyValue(m_algPropName.toStdString(), name.toStdString());
    } catch (std::invalid_argument &) {
      return false;
    }
  } else if ((!m_workspaceTypes.empty()) && m_workspaceTypes.indexOf(QString::fromStdString(object->id())) == -1) {
    return false;
  } else if (!hasValidSuffix(name)) {
    return false;
  } else if (!hasValidNumberOfBins(object)) {
    return false;
  } else if (!m_showGroups) {
    auto group = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(object);
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
  clear();
  if (m_optional)
    addItem("");
  auto &ads = Mantid::API::AnalysisDataService::Instance();
  std::vector<std::string> items;
  if (showHiddenWorkspaces()) {
    items = ads.getObjectNames(Mantid::Kernel::DataServiceSort::Sorted, Mantid::Kernel::DataServiceHidden::Include);
  } else {
    items = ads.getObjectNames();
  }

  QStringList namesToAdd;
  for (auto &item : items) {
    QString name = QString::fromStdString(item);
    if (checkEligibility(name, ads.retrieve(item))) {
      namesToAdd << name;
    }
  }
  addItems(namesToAdd);
}

/**
 * Called when an item is dropped
 * @param de :: the drop event data package
 */
void WorkspaceMultiSelector::dropEvent(QDropEvent *de) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  const QMimeData *mimeData = de->mimeData();
  QString text = mimeData->text();
  int equal_pos = text.indexOf("=");
  QString ws_name = text.left(equal_pos - 1);
  QString ws_name_test = text.mid(equal_pos + 7, equal_pos - 1);
  /* FOR THE MOMENT DONT FIDDLE WITH THIS UNTILS IS CLEARER HOW TO RUN IT
  if (ws_name == ws_name_test) {
    int index = findText(ws_name);
    if (index >= 0) {
      setCurrentIndex(index);
      de->acceptProposedAction();
    }
  } */
}

/**
 * Called when an item is dragged onto a control
 * @param de :: the drag event data package
 */
void WorkspaceMultiSelector::dragEnterEvent(QDragEnterEvent *de) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  const QMimeData *mimeData = de->mimeData();
  if (mimeData->hasText()) {
    QString text = mimeData->text();
    if (text.contains(" = mtd[\""))
      de->acceptProposedAction();
  }
}

/**
 * Called when there is an interaction with the widget.
 */
void WorkspaceMultiSelector::focusInEvent(QFocusEvent *) { emit focussed(); }
