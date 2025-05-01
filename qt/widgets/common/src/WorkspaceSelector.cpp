// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidQtWidgets/Common/WorkspaceSelector.h"

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
WorkspaceSelector::WorkspaceSelector(QWidget *parent, bool init)
    : QComboBox(parent), m_addObserver(*this, &WorkspaceSelector::handleAddEvent),
      m_remObserver(*this, &WorkspaceSelector::handleRemEvent),
      m_clearObserver(*this, &WorkspaceSelector::handleClearEvent),
      m_renameObserver(*this, &WorkspaceSelector::handleRenameEvent),
      m_replaceObserver(*this, &WorkspaceSelector::handleReplaceEvent), m_init(init), m_workspaceTypes(),
      m_showHidden(false), m_showGroups(true), m_optional(false), m_sorted(false), m_binLimits(std::make_pair(0, -1)),
      m_suffix(), m_algName(), m_algPropName(), m_algorithm() {
  setEditable(true);
  if (init) {
    connectObservers();
  }
  this->setAcceptDrops(true);
  this->completer()->setCompletionMode(QCompleter::PopupCompletion);
  this->setInsertPolicy(QComboBox::NoInsert);
}

/**
 * Destructor for WorkspaceSelector
 * De-subscribes this object from the Poco NotificationCentre
 */
WorkspaceSelector::~WorkspaceSelector() { disconnectObservers(); }

/**
 * De-subscribes this object from the Poco NotificationCentre
 */
void WorkspaceSelector::disconnectObservers() {
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
void WorkspaceSelector::connectObservers() {
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

QStringList WorkspaceSelector::getWorkspaceTypes() const { return m_workspaceTypes; }

void WorkspaceSelector::setWorkspaceTypes(const QStringList &types) {
  if (types != m_workspaceTypes) {
    m_workspaceTypes = types;
    if (m_init) {
      refresh();
    }
  }
}

bool WorkspaceSelector::showHiddenWorkspaces() const { return m_showHidden; }

void WorkspaceSelector::showHiddenWorkspaces(bool show) {
  if (show != m_showHidden) {
    m_showHidden = show;
    if (m_init) {
      refresh();
    }
  }
}

bool WorkspaceSelector::showWorkspaceGroups() const { return m_showGroups; }

void WorkspaceSelector::showWorkspaceGroups(bool show) {
  if (show != m_showGroups) {
    m_showGroups = show;
    if (m_init) {
      refresh();
    }
  }
}

bool WorkspaceSelector::isValid() const { return (this->currentText() != ""); }

bool WorkspaceSelector::isOptional() const { return m_optional; }

void WorkspaceSelector::setOptional(bool optional) {
  if (optional != m_optional) {
    m_optional = optional;
    if (m_init)
      refresh();
  }
}

bool WorkspaceSelector::isSorted() const { return m_sorted; }

void WorkspaceSelector::setSorted(bool sorted) {
  if (sorted != m_sorted) {
    m_sorted = sorted;
    if (m_init)
      refresh();
  }
}

bool WorkspaceSelector::isConnected() const { return m_connected; }

QStringList WorkspaceSelector::getSuffixes() const { return m_suffix; }

void WorkspaceSelector::setSuffixes(const QStringList &suffix) {
  if (suffix != m_suffix) {
    m_suffix = suffix;
    if (m_init) {
      refresh();
    }
  }
}

void WorkspaceSelector::setLowerBinLimit(int numberOfBins) { m_binLimits.first = numberOfBins; }

void WorkspaceSelector::setUpperBinLimit(int numberOfBins) { m_binLimits.second = numberOfBins; }

QString WorkspaceSelector::getValidatingAlgorithm() const { return m_algName; }

void WorkspaceSelector::setValidatingAlgorithm(const QString &algName) {
  if (algName == m_algName) {
    return;
  }
  m_algName = algName;
  if (m_init) {
    m_algorithm = Mantid::API::AlgorithmManager::Instance().createUnmanaged(algName.toStdString());
    m_algorithm->initialize();
    std::vector<Mantid::Kernel::Property *> props = m_algorithm->getProperties();
    for (auto &prop : props) {
      if (prop->direction() == Mantid::Kernel::Direction::Input) {
        // try to cast property to WorkspaceProperty
        const Mantid::API::WorkspaceProperty<> *wsProp = dynamic_cast<Mantid::API::WorkspaceProperty<> *>(prop);
        if (wsProp != nullptr) {
          m_algPropName = QString::fromStdString(prop->name());
          break;
        }
      }
    }
    refresh();
  }
}

void WorkspaceSelector::handleAddEvent(Mantid::API::WorkspaceAddNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  if (!showHiddenWorkspaces() &&
      Mantid::API::AnalysisDataService::Instance().isHiddenDataServiceObject(pNf->objectName())) {
    return;
  }
  QString name = QString::fromStdString(pNf->objectName());
  if (checkEligibility(name, pNf->object())) {

    addItem(name);
    if (isSorted()) {
      model()->sort(0);
    }
  }
}

void WorkspaceSelector::handleRemEvent(Mantid::API::WorkspacePostDeleteNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  QString name = QString::fromStdString(pNf->objectName());
  int index = findText(name);
  if (index != -1) {
    removeItem(index);
  }
  if (currentIndex() == -1) {
    emit emptied();
  }
}

void WorkspaceSelector::handleClearEvent(Mantid::API::ClearADSNotification_ptr /*unused*/) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  this->clear();
  if (m_optional)
    addItem("");
  emit emptied();
}

void WorkspaceSelector::handleRenameEvent(Mantid::API::WorkspaceRenameNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  QString name = QString::fromStdString(pNf->objectName());
  QString newName = QString::fromStdString(pNf->newObjectName());
  auto &ads = Mantid::API::AnalysisDataService::Instance();

  bool eligible = checkEligibility(newName, ads.retrieve(pNf->newObjectName()));
  int index = findText(name);
  int newIndex = findText(newName);

  if (eligible) {
    if (index != -1 && newIndex == -1) {
      this->setItemText(index, newName);
      if (isSorted()) {
        model()->sort(0);
      }
    } else if (index == -1 && newIndex == -1) {
      addItem(newName);
      if (isSorted()) {
        model()->sort(0);
      }
    } else
      removeItem(index);
  } else {
    if (index != -1) {
      removeItem(index);
    }
  }
}

void WorkspaceSelector::handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  QString name = QString::fromStdString(pNf->objectName());
  auto &ads = Mantid::API::AnalysisDataService::Instance();

  bool eligible = checkEligibility(name, ads.retrieve(pNf->objectName()));
  int index = findText(name);

  // if it is inside and it is eligible do nothing
  // if it is not inside and it is eligible insert
  // if it is inside and it is not eligible remove
  // if it is not inside and it is not eligible do nothing
  bool inside = (index != -1);
  if ((inside && eligible) || (!inside && !eligible))
    return;
  else if (!inside && eligible) {
    addItem(name);
    if (isSorted()) {
      model()->sort(0);
    }
  } else // (inside && !eligible)
    removeItem(index);
}

bool WorkspaceSelector::checkEligibility(const QString &name, const Mantid::API::Workspace_sptr &object) const {
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

bool WorkspaceSelector::hasValidSuffix(const QString &name) const {
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

bool WorkspaceSelector::hasValidNumberOfBins(const Mantid::API::Workspace_sptr &object) const {
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

void WorkspaceSelector::refresh() {
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
  this->addItems(namesToAdd);
  if (isSorted()) {
    model()->sort(0);
  }
}

/**
 * Called when an item is dropped
 * @param de :: the drop event data package
 */
void WorkspaceSelector::dropEvent(QDropEvent *de) {
  const std::lock_guard<std::mutex> lock(m_adsMutex);
  const QMimeData *mimeData = de->mimeData();
  QString text = mimeData->text();
  int equal_pos = text.indexOf("=");
  QString ws_name = text.left(equal_pos - 1);
  QString ws_name_test = text.mid(equal_pos + 7, equal_pos - 1);

  if (ws_name == ws_name_test) {
    int index = findText(ws_name);
    if (index >= 0) {
      setCurrentIndex(index);
      de->acceptProposedAction();
    }
  }
}

/**
 * Called when an item is dragged onto a control
 * @param de :: the drag event data package
 */
void WorkspaceSelector::dragEnterEvent(QDragEnterEvent *de) {
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
void WorkspaceSelector::focusInEvent(QFocusEvent *) { emit focussed(); }
