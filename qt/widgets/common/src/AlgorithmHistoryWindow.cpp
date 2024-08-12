// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Strings.h"

#include "MantidQtWidgets/Common/AlgorithmHistoryWindow.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QTextStream>

#include <cstdio>
#include <fstream>
#include <numeric>
#include <utility>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
/// static history window logger
Mantid::Kernel::Logger window_log("AlgorithmHistoryWindow");
/// static tree widget logger
Mantid::Kernel::Logger widget_log("AlgHistoryTreeWidget");

int getNumberOfItemsInTree(QTreeWidgetItem *item) {
  // item is the root of the tree
  int count{1};
  for (int i = 0; i < item->childCount(); i++) {
    if (item->child(i)->checkState(1) == Qt::Checked) {
      // recurse into unrolled algorithms
      count += getNumberOfItemsInTree(item->child(i));
    } else {
      count++;
    }
  }
  return count;
}
} // namespace

AlgExecSummaryGrpBox::AlgExecSummaryGrpBox(QWidget *w)
    : QGroupBox(w), m_execDurationlabel(nullptr), m_execDurationEdit(nullptr), m_Datelabel(nullptr),
      m_execDateTimeEdit(nullptr), m_algexecDuration() {}

AlgExecSummaryGrpBox::AlgExecSummaryGrpBox(const QString &title, QWidget *w)
    : QGroupBox(title, w), m_execDurationlabel(nullptr), m_execDurationEdit(nullptr), m_Datelabel(nullptr),
      m_execDateTimeEdit(nullptr), m_algexecDuration() {

  m_execDurationEdit = new QLineEdit("", this);
  if (m_execDurationEdit)
    m_execDurationEdit->setReadOnly(true);
  m_execDurationlabel = new QLabel("Duration:", this);
  if (m_execDurationlabel)
    m_execDurationlabel->setBuddy(m_execDurationEdit);

  QDateTime datetime(QDate(0, 0, 0), QTime(0, 0, 0), Qt::LocalTime);
  m_execDateTimeEdit = new QLineEdit("", this);
  if (m_execDateTimeEdit)
    m_execDateTimeEdit->setReadOnly(true);
  m_Datelabel = new QLabel("Date:", this);
  if (m_Datelabel)
    m_Datelabel->setBuddy(m_execDateTimeEdit);

  auto *formLayout = new QFormLayout;
  if (formLayout) {
    formLayout->addRow(m_execDurationlabel, m_execDurationEdit);
    formLayout->addRow(m_Datelabel, m_execDateTimeEdit);
    setLayout(formLayout);
  }
  setGeometry(5, 210, 205, 130);
}
AlgExecSummaryGrpBox::~AlgExecSummaryGrpBox() {
  if (m_execDurationlabel) {
    delete m_execDurationlabel;
    m_execDurationlabel = nullptr;
  }
  if (m_execDurationEdit) {
    delete m_execDurationEdit;
    m_execDurationEdit = nullptr;
  }
  if (m_Datelabel) {
    delete m_Datelabel;
    m_Datelabel = nullptr;
  }
  if (m_execDateTimeEdit) {
    delete m_execDateTimeEdit;
    m_execDateTimeEdit = nullptr;
  }
}
void AlgExecSummaryGrpBox::setData(const double execDuration, const Mantid::Types::Core::DateAndTime execDate) {
  QString dur("");
  dur.setNum(execDuration, 'g', 6);
  dur += " seconds";
  QLineEdit *execDurationEdit = getAlgExecDurationCtrl();
  if (execDurationEdit)
    execDurationEdit->setText(dur);

  // Get the timeinfo structure, but converting from UTC to local time
  std::tm t = execDate.to_localtime_tm();
  QTime qt(t.tm_hour, t.tm_min, t.tm_sec);
  QDate qd(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  QDateTime datetime(qd, qt, Qt::LocalTime);

  QString str("");
  str = datetime.toString("dd/MM/yyyy hh:mm:ss");

  QLineEdit *datetimeEdit = getAlgExecDateCtrl();
  if (datetimeEdit)
    datetimeEdit->setText(str);
}

AlgEnvHistoryGrpBox::AlgEnvHistoryGrpBox(QWidget *w)
    : QGroupBox(w), m_osNameLabel(nullptr), m_osNameEdit(nullptr), m_osVersionLabel(nullptr), m_osVersionEdit(nullptr),
      m_frmworkVersionLabel(nullptr), m_frmwkVersnEdit(nullptr) {}

AlgEnvHistoryGrpBox::AlgEnvHistoryGrpBox(const QString &title, QWidget *w)
    : QGroupBox(title, w), m_osNameLabel(nullptr), m_osNameEdit(nullptr), m_osVersionLabel(nullptr),
      m_osVersionEdit(nullptr), m_frmworkVersionLabel(nullptr), m_frmwkVersnEdit(nullptr) {
  // OS Name Label & Edit Box
  m_osNameEdit = new QLineEdit("", this);
  if (m_osNameEdit) {
    m_osNameEdit->setReadOnly(true);
  }
  m_osNameLabel = new QLabel("OS Name:", this);
  if (m_osNameLabel)
    m_osNameLabel->setBuddy(m_osNameEdit);

  // OS Version Label & Edit Box
  m_osVersionEdit = new QLineEdit("", this);
  if (m_osVersionEdit) {
    m_osVersionEdit->setReadOnly(true);
    m_osVersionLabel = new QLabel("OS Version:", this);
  }
  if (m_osVersionLabel)
    m_osVersionLabel->setBuddy(m_osVersionEdit);

  // Mantid Framework Version Label & Edit Box
  m_frmwkVersnEdit = new QLineEdit("", this);
  if (m_frmwkVersnEdit)
    m_frmwkVersnEdit->setReadOnly(true);
  m_frmworkVersionLabel = new QLabel("Framework Version:", this);
  if (m_frmworkVersionLabel)
    m_frmworkVersionLabel->setBuddy(m_frmwkVersnEdit);

  auto *formLayout = new QFormLayout();
  if (formLayout) {
    formLayout->addRow(m_osNameLabel, m_osNameEdit);
    formLayout->addRow(m_osVersionLabel, m_osVersionEdit);
    formLayout->addRow(m_frmworkVersionLabel, m_frmwkVersnEdit);
    setLayout(formLayout);
  }
  setGeometry(214, 210, 347, 130);
}
AlgEnvHistoryGrpBox::~AlgEnvHistoryGrpBox() {
  if (m_osNameLabel) {
    delete m_osNameLabel;
    m_osNameLabel = nullptr;
  }
  if (m_osNameEdit) {
    delete m_osNameEdit;
    m_osNameEdit = nullptr;
  }
  if (m_osVersionLabel) {
    delete m_osVersionLabel;
    m_osVersionLabel = nullptr;
  }
  if (m_osVersionEdit) {
    delete m_osVersionEdit;
    m_osVersionEdit = nullptr;
  }
  if (m_frmworkVersionLabel) {
    delete m_frmworkVersionLabel;
    m_frmworkVersionLabel = nullptr;
  }
  if (m_frmwkVersnEdit) {
    delete m_frmwkVersnEdit;
    m_frmwkVersnEdit = nullptr;
  }
}

AlgorithmHistoryWindow::AlgorithmHistoryWindow(QWidget *parent, const std::shared_ptr<const Workspace> &wsptr)
    : QDialog(parent), m_algHist(wsptr->getHistory()), m_histPropWindow(nullptr), m_execSumGrpBox(nullptr),
      m_envHistGrpBox(nullptr), m_wsName(wsptr->getName().c_str()), m_view(wsptr->getHistory().createView()) {

  if (m_algHist.empty()) {
    throw std::invalid_argument("No history found on the given workspace");
  }

  setWindowTitle(tr("Algorithm History"));
  setMinimumHeight(500);
  setMinimumWidth(750);
  resize(540, 380);

#ifdef Q_OS_MAC
  // Work around to ensure that floating windows remain on top of the main
  // application window, but below other applications on Mac
  // Note: Qt::Tool cannot have both a max and min button on OSX
  Qt::WindowFlags flags = windowFlags();
  flags |= Qt::Tool;
  flags |= Qt::CustomizeWindowHint;
  flags |= Qt::WindowMinimizeButtonHint;
  flags |= Qt::WindowCloseButtonHint;
  setWindowFlags(flags);
#endif

  // Create a tree widget to display the algorithm names in the workspace
  // history
  m_Historytree = new AlgHistoryTreeWidget(this);
  if (m_Historytree) {
    QStringList headers;
    headers << "Algorithms"
            << "Unroll";

    m_Historytree->setColumnCount(2);
    m_Historytree->setColumnWidth(0, 180);
    m_Historytree->setColumnWidth(1, 55);
    m_Historytree->setHeaderLabels(headers);
    m_Historytree->setGeometry(5, 5, 205, 200);
    // Populate the History Tree widget
    m_Historytree->populateAlgHistoryTreeWidget(m_algHist);
  }

  // create a tree widget to display history properties
  if (!m_histPropWindow)
    m_histPropWindow = createAlgHistoryPropWindow();

  // connect history tree with window
  connect(m_Historytree, SIGNAL(updateAlgorithmHistoryWindow(Mantid::API::AlgorithmHistory_const_sptr)), this,
          SLOT(updateAll(Mantid::API::AlgorithmHistory_const_sptr)));
  connect(m_Historytree, SIGNAL(unrollAlgorithmHistory(const std::vector<int> &)), this,
          SLOT(doUnroll(const std::vector<int> &)));
  connect(m_Historytree, SIGNAL(rollAlgorithmHistory(int)), this, SLOT(doRoll(int)));

  // The tree and the history details layout
  auto *treeLayout = new QHBoxLayout;
  treeLayout->addWidget(m_Historytree, 3); // History stretches 1
  treeLayout->addWidget(m_histPropWindow->m_histpropTree,
                        5); // Properties gets more space

  // Create a GroupBox to display exec date,duration
  if (!m_execSumGrpBox)
    m_execSumGrpBox = createExecSummaryGrpBox();
  // Create a Groupbox to display environment details
  if (!m_envHistGrpBox)
    m_envHistGrpBox = createEnvHistGrpBox(wsptr->getHistory().getEnvironmentHistory());

  auto *environmentLayout = new QHBoxLayout;
  environmentLayout->addWidget(m_execSumGrpBox, 1);
  environmentLayout->addWidget(m_envHistGrpBox, 2);

  // The buttons at the bottom
  m_scriptVersionLabel = new QLabel("Algorithm Versions:", this);
  m_scriptComboMode = new QComboBox(this);
  // N.B. The combobox item strings below are used in
  // AlgorithmHistoryWindow::getScriptVersionMode()
  // If you change them here, you MUST change them there too.
  m_scriptComboMode->addItem("Only Specify Old Versions");
  m_scriptComboMode->addItem("Never Specify Versions");
  m_scriptComboMode->addItem("Always Specify Versions");
  m_scriptComboMode->setToolTip("When to specify which version of an algorithm was used.");
  m_scriptButtonFile = new QPushButton("Script to File", this);
  m_scriptButtonClipboard = new QPushButton("Script to Clipboard", this);
  connect(m_scriptButtonFile, SIGNAL(clicked()), this, SLOT(writeToScriptFile()));
  connect(m_scriptButtonClipboard, SIGNAL(clicked()), this, SLOT(copytoClipboard()));

  auto *buttonLayout = new QHBoxLayout;
  buttonLayout->addStretch(1); // Align the button to the right
  buttonLayout->addWidget(m_scriptVersionLabel);
  buttonLayout->addWidget(m_scriptComboMode);
  buttonLayout->addWidget(m_scriptButtonFile);
  buttonLayout->addWidget(m_scriptButtonClipboard);

  // Unroll all checkbox below tree layout
  m_unrollAllHistoryCheckbox = new QCheckBox("Unroll All Algorithms", this);
  connect(m_unrollAllHistoryCheckbox, SIGNAL(stateChanged(int)), this, SLOT(unrollAll(int)));

  // Main layout
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(treeLayout);
  mainLayout->addWidget(m_unrollAllHistoryCheckbox);
  mainLayout->addLayout(environmentLayout);
  mainLayout->addLayout(buttonLayout);
}

AlgorithmHistoryWindow::AlgorithmHistoryWindow(QWidget *parent, const QString &workspaceName)
    : AlgorithmHistoryWindow(parent,
                             AnalysisDataService::Instance().retrieveWS<const Workspace>(workspaceName.toStdString())) {
}

AlgorithmHistoryWindow::~AlgorithmHistoryWindow() {
  if (m_Historytree) {
    delete m_Historytree;
    m_Historytree = nullptr;
  }
  if (m_histPropWindow) {
    delete m_histPropWindow;
    m_histPropWindow = nullptr;
  }
  if (m_execSumGrpBox) {
    delete m_execSumGrpBox;
    m_execSumGrpBox = nullptr;
  }
  if (m_envHistGrpBox) {
    delete m_envHistGrpBox;
    m_envHistGrpBox = nullptr;
  }
}

// Delete window object on close
// Without this the windows stay in memory when closed in workbench
void AlgorithmHistoryWindow::closeEvent(QCloseEvent *ce) {
  this->deleteLater();
  ce->accept();
}

AlgExecSummaryGrpBox *AlgorithmHistoryWindow::createExecSummaryGrpBox() {
  AlgExecSummaryGrpBox *pgrpBox = new AlgExecSummaryGrpBox("Execution Summary", this);
  if (pgrpBox) {
    // iterating through algorithm history to display exec duration,date
    // last executed algorithm exec duration,date will be displayed in gruopbox
    const size_t noEntries = m_algHist.size();
    for (size_t i = 0; i < noEntries; ++i) {
      const auto entry = m_algHist.getAlgorithmHistory(i);
      double duration = 0;
      duration = entry->executionDuration();
      Mantid::Types::Core::DateAndTime date = entry->executionDate();
      pgrpBox->setData(duration, date);
    }
    return pgrpBox;
  } else {
    QMessageBox::critical(this, "Mantid", "Invalid Pointer");
    return nullptr;
  }
}
AlgEnvHistoryGrpBox *AlgorithmHistoryWindow::createEnvHistGrpBox(const EnvironmentHistory &envHist) {
  AlgEnvHistoryGrpBox *pEnvGrpBox = new AlgEnvHistoryGrpBox("Environment History", this);
  if (pEnvGrpBox) {
    pEnvGrpBox->fillEnvHistoryGroupBox(envHist);
    return pEnvGrpBox;
  } else {
    QMessageBox::critical(this, "Mantid", "Invalid Pointer");
    return nullptr;
  }
}
AlgHistoryProperties *AlgorithmHistoryWindow::createAlgHistoryPropWindow() {
  std::vector<PropertyHistory_sptr> histProp;
  const Mantid::API::AlgorithmHistories &entries = m_algHist.getAlgorithmHistories();
  auto rIter = entries.rbegin();
  histProp = (*rIter)->getProperties();

  // AlgHistoryProperties * phistPropWindow=new
  // AlgHistoryProperties(this,m_algHist);
  if (histProp.empty()) {
    QMessageBox::critical(this, "Mantid", "Properties not set");
    return nullptr;
  }
  AlgHistoryProperties *phistPropWindow = new AlgHistoryProperties(this, histProp);
  if (phistPropWindow) {
    phistPropWindow->displayAlgHistoryProperties();
    return phistPropWindow;
  } else {
    QMessageBox::critical(this, "Mantid", "Invalid Pointer");
    return nullptr;
  }
}

//! Used by the save script to clipboard/file buttons to select which versioning
// mode to use.
std::string AlgorithmHistoryWindow::getScriptVersionMode() {
  std::string curText = m_scriptComboMode->currentText().toStdString();

  if (curText == "Only Specify Old Versions") {
    return "old";
  } else if (curText == "Always Specify Versions") {
    return "all";
  } else if (curText == "Never Specify Versions") {
    return "none";
  }

  throw std::runtime_error("AlgorithmHistoryWindow::getScriptVersionMode "
                           "received unhandled version mode string");
}

void AlgorithmHistoryWindow::writeToScriptFile() {
  QString prevDir = MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString scriptDir("");
  // Default script directory
  if (prevDir.isEmpty()) {
    scriptDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory"));
  } else {
    scriptDir = prevDir;
  }
  QString filePath = QFileDialog::getSaveFileName(this, tr("Save Script As "), scriptDir, tr("Script files (*.py)"));
  // An empty string indicates they clicked cancel
  if (filePath.isEmpty())
    return;

  // fix for 34451 in Linux, unlike native filedialog, QFileDialog will not add file extension by type
  // This only occur in Linux as in Win and Mac QFileDialog calls the native filedialog
  if (!filePath.endsWith(".py"))
    filePath += ".py";

  ScriptBuilder builder(m_view, getScriptVersionMode());
  std::ofstream file(filePath.toStdString().c_str(), std::ofstream::trunc);
  file << builder.build();
  file.flush();
  file.close();

  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filePath).absoluteDir().path());
}

void AlgEnvHistoryGrpBox::fillEnvHistoryGroupBox(const EnvironmentHistory &envHistory) {
  QLineEdit *osNameEdit = getosNameEdit();
  if (osNameEdit) {
    std::string osname = envHistory.osName();
    osNameEdit->setText(osname.c_str());
  }

  QLineEdit *osVersionEdit = getosVersionEdit();
  if (osVersionEdit) {
    std::string osversion = envHistory.osVersion();
    osVersionEdit->setText(osversion.c_str());
  }

  QLineEdit *frmwkVersnEdit = getfrmworkVersionEdit();
  if (frmwkVersnEdit) {
    std::string frwkversn = envHistory.frameworkVersion();
    frmwkVersnEdit->setText(frwkversn.c_str());
  }
}

void AlgorithmHistoryWindow::updateAll(const Mantid::API::AlgorithmHistory_const_sptr &algHistory) {
  updateAlgHistoryProperties(algHistory);
  updateExecSummaryGrpBox(algHistory);
}

void AlgorithmHistoryWindow::updateAlgHistoryProperties(const AlgorithmHistory_const_sptr &algHistory) {
  PropertyHistories histProp = algHistory->getProperties();
  if (m_histPropWindow) {
    m_histPropWindow->setAlgProperties(histProp);
    m_histPropWindow->clearData();
    m_histPropWindow->displayAlgHistoryProperties();
  }
}

void AlgorithmHistoryWindow::updateExecSummaryGrpBox(const AlgorithmHistory_const_sptr &algHistory) {
  // getting the selected algorithm at pos from History vector
  double duration = algHistory->executionDuration();
  Mantid::Types::Core::DateAndTime date = algHistory->executionDate();
  if (m_execSumGrpBox)
    m_execSumGrpBox->setData(duration, date);
}

void AlgorithmHistoryWindow::copytoClipboard() {
  ScriptBuilder builder(m_view, getScriptVersionMode());
  QString script;
  const std::string contents = builder.build();
  script.append(contents.c_str());

  // Send to clipboard.
  QClipboard *clipboard = QApplication::clipboard();
  if (nullptr != clipboard) {
    clipboard->setText(script);
  }
}

void AlgorithmHistoryWindow::doUnroll(const std::vector<int> &unrollIndicies) {
  for (const auto &unrollIndex : unrollIndicies) {
    m_view->unroll(unrollIndex);
  }
}

void AlgorithmHistoryWindow::doRoll(int index) { m_view->roll(index); }

void AlgorithmHistoryWindow::unrollAll(int state) {
  // Iterate all items in tree which have child algorithms to be unrolled
  QTreeWidgetItemIterator it(m_Historytree, QTreeWidgetItemIterator::HasChildren);
  while (*it) {
    // set state of unroll based on checkbox sate
    if (state == Qt::Checked)
      (*it)->setCheckState(1, Qt::Checked);
    else
      (*it)->setCheckState(1, Qt::Unchecked);
    ++it;
  }
}

//--------------------------------------------------------------------------------------------------
// AlgHistoryProperties Definitions
//--------------------------------------------------------------------------------------------------

AlgHistoryProperties::AlgHistoryProperties(QWidget *w, std::vector<PropertyHistory_sptr> propHist)
    : m_Histprop(std::move(propHist)) {
  QStringList hList;
  hList << "Name"
        << "Value"
        << "Default?:"
        << "Direction";

  m_histpropTree = new QTreeWidget(w);
  m_histpropTree->setTextElideMode(Qt::ElideMiddle);
  m_histpropTree->setColumnCount(4);
  m_histpropTree->setSelectionMode(QAbstractItemView::NoSelection);
  m_histpropTree->setHeaderLabels(hList);
  m_histpropTree->setGeometry(213, 5, 350, 200);

  m_contextMenu = new QMenu(w);

  m_copyAction = new QAction("Copy to Clipboard", m_contextMenu);
  connect(m_copyAction, SIGNAL(triggered()), this, SLOT(copySelectedItemText()));
  m_contextMenu->addAction(m_copyAction);

  m_histpropTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_histpropTree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));
}

void AlgHistoryProperties::clearData() {
  if (m_histpropTree) {
    m_histpropTree->clear();
    int ntopcount = m_histpropTree->topLevelItemCount();
    while (ntopcount--) {
      m_histpropTree->topLevelItem(ntopcount);
    }
  }
}

void AlgHistoryProperties::setAlgProperties(const std::vector<PropertyHistory_sptr> &histProp) {
  m_Histprop.assign(histProp.begin(), histProp.end());
}

const PropertyHistories &AlgHistoryProperties::getAlgProperties() { return m_Histprop; }

void AlgHistoryProperties::popupMenu(const QPoint &pos) {
  QTreeWidgetItem *treeItem = m_histpropTree->itemAt(pos);
  if (!treeItem)
    return;

  m_selectedItemText = treeItem->text(m_histpropTree->currentColumn());
  m_contextMenu->popup(QCursor::pos());
}

void AlgHistoryProperties::copySelectedItemText() {
  QClipboard *clipboard = QApplication::clipboard();
  if (clipboard)
    clipboard->setText(m_selectedItemText);
}

/**
 * @brief Populates the Algorithm History display
 * with property names, values, directions and whether their values were the
 * defaults.
 *
 * If a value was unset and its default value is EMPTY_INT, EMPTY_DBL
 * or EMPTY_LONG, display an empty space to the user rather than the
 * internal numeric representation of an empty value.
 *
 */
void AlgHistoryProperties::displayAlgHistoryProperties() {
  QStringList propList;
  for (std::vector<PropertyHistory_sptr>::const_iterator pIter = m_Histprop.begin(); pIter != m_Histprop.end();
       ++pIter) {
    std::string sProperty = (*pIter)->name();
    propList.append(sProperty.c_str());

    sProperty = (*pIter)->value();
    bool bisDefault = (*pIter)->isDefault();
    if (bisDefault == true) {
      if ((*pIter)->isEmptyDefault()) {
        sProperty = ""; // replace EMPTY_XXX with empty string
      }
    }
    propList.append(sProperty.c_str());

    bisDefault ? (sProperty = "Yes") : (sProperty = "No");
    propList.append(sProperty.c_str());
    int nDirection = (*pIter)->direction();
    switch (nDirection) {
    case 0: {
      sProperty = "Input";
      break;
    }
    case 1: {
      sProperty = "Output";
      break;
    }
    case 2: {
      sProperty = "InOut";
      break;
    }
    default: {
      sProperty = "N/A";
      break;
    }
    }
    propList.append(sProperty.c_str());
    QTreeWidgetItem *item = new QTreeWidgetItem(propList);
    if (m_histpropTree)
      m_histpropTree->addTopLevelItem(item);
    propList.clear();

  } // end of properties for loop

  // Fit some column widths to data
  m_histpropTree->resizeColumnToContents(0); // Property name
  m_histpropTree->resizeColumnToContents(2); // Default
  m_histpropTree->resizeColumnToContents(3); // Direction
}

//--------------------------------------------------------------------------------------------------
// AlgHistoryTreeWidget Definitions
//--------------------------------------------------------------------------------------------------
void AlgHistoryTreeWidget::onItemChanged(QTreeWidgetItem *item, int index) {
  this->blockSignals(true);
  if (index == UNROLL_COLUMN_INDEX && item->checkState(index) == Qt::Checked) {
    itemChecked(item, index);
  } else if (index == UNROLL_COLUMN_INDEX && item->checkState(index) == Qt::Unchecked) {
    itemUnchecked(item, index);
  }
  this->blockSignals(false);
}

void AlgHistoryTreeWidget::getItemIndex(QTreeWidgetItem *item, int &index) {
  // Counts all of the algorithms which precede the given one, including child
  // algorithms if an algorithm is unrolled.
  QModelIndex modelIndex;
  if (item->parent()) {
    index++;
  }
  modelIndex = indexFromItem(item, 1);
  for (auto i = 0; i < modelIndex.row(); i++) {
    // If an algorithm is unrolled, add all of its children to the index,
    // otherwise just add one.
    if (item->parent() && item->parent()->child(i)->checkState(1) == Qt::Checked) {
      index += getNumberOfItemsInTree(item->parent()->child(i));
    } else if (item->parent() == nullptr && topLevelItem(i)->checkState(1) == Qt::Checked) {
      index += getNumberOfItemsInTree(topLevelItem(i));
    } else {
      index += 1;
    }
  }
}

void AlgHistoryTreeWidget::itemChecked(QTreeWidgetItem *item, int index) {
  std::vector<QTreeWidgetItem *> checkedItems;
  std::vector<int> unrollIndices;
  int unrollIndex{0};

  for (auto currentItem = item; currentItem; currentItem = currentItem->parent()) {
    // Check the item if it isn't already checked.
    if (currentItem->checkState(index) != Qt::Checked) {
      currentItem->setCheckState(index, Qt::Checked);
    }

    checkedItems.emplace_back(currentItem);
  }

  for (auto it = checkedItems.rbegin(); it != checkedItems.rend(); ++it) {
    auto currentItem = *it;

    // Find where we are in the tree.
    getItemIndex(currentItem, unrollIndex);

    unrollIndices.emplace_back(unrollIndex);
  }
  this->blockSignals(false);
  emit unrollAlgorithmHistory(unrollIndices);
  this->blockSignals(true);
}

void AlgHistoryTreeWidget::itemUnchecked(QTreeWidgetItem *item, int index) {
  int rollIndex{0};

  // disable any children
  uncheckAllChildren(item, index);

  // find where we are in the tree
  for (auto currentItem = item; currentItem; currentItem = currentItem->parent()) {
    getItemIndex(currentItem, rollIndex);
  }

  this->blockSignals(false);
  emit rollAlgorithmHistory(rollIndex);
  this->blockSignals(true);
}

void AlgHistoryTreeWidget::uncheckAllChildren(QTreeWidgetItem *item, int index) {
  if (item->childCount() > 0) {
    item->setCheckState(index, Qt::Unchecked);
    for (int i = 0; i < item->childCount(); ++i) {
      uncheckAllChildren(item->child(i), index);
    }
  }
}

void AlgHistoryTreeWidget::treeSelectionChanged() {
  if (AlgHistoryItem *item = dynamic_cast<AlgHistoryItem *>(this->selectedItems()[0])) {
    emit updateAlgorithmHistoryWindow(item->getAlgorithmHistory());
  }
}

void AlgHistoryTreeWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
  QTreeView::selectionChanged(selected, deselected);
  treeSelectionChanged();
}

void AlgHistoryTreeWidget::populateAlgHistoryTreeWidget(const WorkspaceHistory &wsHist) {
  this->blockSignals(true);
  const Mantid::API::AlgorithmHistories &entries = wsHist.getAlgorithmHistories();
  auto algHistIter = entries.begin();

  for (; algHistIter != entries.end(); ++algHistIter) {
    int nAlgVersion = (*algHistIter)->version();
    const QString algName = concatVersionwithName((*algHistIter)->name(), nAlgVersion);

    AlgHistoryItem *item = new AlgHistoryItem(QStringList(algName), *algHistIter);
    this->addTopLevelItem(item);
    populateNestedHistory(item, *algHistIter);
  }
  resizeColumnToContents(0); // Algorithm name
  resizeColumnToContents(1); // Unroll
  this->blockSignals(false);
}

void AlgHistoryTreeWidget::populateNestedHistory(AlgHistoryItem *parentWidget,
                                                 const Mantid::API::AlgorithmHistory_sptr &history) {
  const Mantid::API::AlgorithmHistories &entries = history->getChildHistories();
  if (history->childHistorySize() > 0) {
    parentWidget->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    parentWidget->setCheckState(1, Qt::Unchecked);
  }

  for (const auto &entry : entries) {
    int nAlgVersion = entry->version();
    const QString algName = concatVersionwithName(entry->name(), nAlgVersion);

    AlgHistoryItem *item = new AlgHistoryItem(QStringList(algName), entry, parentWidget);
    parentWidget->addChild(item);
    populateNestedHistory(item, entry);
  }
}

QString AlgHistoryTreeWidget::concatVersionwithName(const std::string &name, const int version) {
  QString algName = name.c_str();
  algName = algName + " v.";
  QString algVersion = QString::number(version, 10);
  algName += algVersion;
  return algName;
}
