//----------------------------------
// Includes
//----------------------------------
#include "MantidSampleLogDialog.h"

// Mantid
#include <MantidAPI/MultipleExperimentInfos.h>
#include "MantidUI.h"

// Qt
#include <QHeaderView>
#include <QRadioButton>
#include <QFormLayout>
#include <QGroupBox>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

//----------------------------------
// Public methods
//----------------------------------
/**
* Construct an object of this type
*	@param wsname :: The name of the workspace object from
*			which to retrieve the log files
*	@param mui :: The MantidUI area
*	@param flags :: Window flags that are passed the the QDialog constructor
*	@param experimentInfoIndex :: optional index in the array of
*        ExperimentInfo objects. Should only be non-zero for MDWorkspaces.
*/
MantidSampleLogDialog::MantidSampleLogDialog(const QString &wsname,
                                             MantidUI *mui, Qt::WFlags flags,
                                             size_t experimentInfoIndex)
    : SampleLogDialogBase(wsname, mui->appWindow(), flags, experimentInfoIndex),
      m_mantidUI(mui) {
  std::stringstream ss;
  ss << "MantidPlot - " << wsname.toStdString().c_str() << " sample logs";
  setWindowTitle(QString::fromStdString(ss.str()));

  QStringList titles;
  titles << "Name"
         << "Type"
         << "Value"
         << "Units";
  m_tree->setHeaderLabels(titles);
  m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  QHeaderView *hHeader = (QHeaderView *)m_tree->header();
  hHeader->setResizeMode(2, QHeaderView::Stretch);
  hHeader->setStretchLastSection(false);

  QHBoxLayout *uiLayout = new QHBoxLayout;
  uiLayout->addWidget(m_tree);

  // ----- Filtering options --------------
  QGroupBox *groupBox = new QGroupBox(tr("Filter log values by"));

  filterNone = new QRadioButton("None");
  filterStatus = new QRadioButton("Status");
  filterPeriod = new QRadioButton("Period");
  filterStatusPeriod = new QRadioButton("Status + Period");
  filterStatusPeriod->setChecked(true);

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(filterNone);
  vbox->addWidget(filterStatus);
  vbox->addWidget(filterPeriod);
  vbox->addWidget(filterStatusPeriod);
  // vbox->addStretch(1);
  groupBox->setLayout(vbox);

  // -------------- Statistics on logs ------------------------
  std::string stats[NUM_STATS] = {
      "Min:", "Max:", "Mean:", "Time Avg:", "Median:", "Std Dev:", "Duration:"};
  QGroupBox *statsBox = new QGroupBox("Log Statistics");
  QFormLayout *statsBoxLayout = new QFormLayout;
  for (size_t i = 0; i < NUM_STATS; i++) {
    statLabels[i] = new QLabel(stats[i].c_str());
    statValues[i] = new QLineEdit("");
    statValues[i]->setReadOnly(1);
    statsBoxLayout->addRow(statLabels[i], statValues[i]);
  }
  statsBox->setLayout(statsBoxLayout);

  QVBoxLayout *hbox = new QVBoxLayout;
  addImportAndCloseButtonsTo(hbox);
  addExperimentInfoSelectorTo(hbox);

  // Finish laying out the right side
  hbox->addWidget(groupBox);
  hbox->addWidget(statsBox);
  hbox->addStretch(1);

  //--- Main layout With 2 sides -----
  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->addLayout(uiLayout, 1); // the tree
  mainLayout->addLayout(hbox, 0);
  // mainLayout->addLayout(bottomButtons);
  this->setLayout(mainLayout);

  init();

  resize(750, 400);

  connect(buttonPlot, SIGNAL(clicked()), this, SLOT(importSelectedLogs()));
  connect(buttonClose, SIGNAL(clicked()), this, SLOT(close()));
  // want a custom context menu
  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(popupMenu(const QPoint &)));

  // Double-click imports a log file
  connect(m_tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
          SLOT(importItem(QTreeWidgetItem *)));

  // Selecting shows the stats of it
  connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this,
          SLOT(showLogStatistics()));

  // Selecting shows the stats of it
  connect(m_tree,
          SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
          this, SLOT(showLogStatistics()));
}

MantidSampleLogDialog::~MantidSampleLogDialog() {}

//------------------------------------------------------------------------------------------------
/**
* Import an item from sample logs
*
*	@param item :: The item to be imported
*	@throw invalid_argument if format identifier for the item is wrong
*/
void MantidSampleLogDialog::importItem(QTreeWidgetItem *item) {
  // used in numeric time series below, the default filter value
  int filter = 0;
  int key = item->data(1, Qt::UserRole).toInt();
  Mantid::Kernel::Property *logData = NULL;
  QString caption = QString::fromStdString(m_wsname) +
                    QString::fromStdString("-") + item->text(0);
  switch (key) {
  case numeric:
  case string:
    m_mantidUI->importString(
        item->text(0), item->data(0, Qt::UserRole).toString(), QString(""),
        QString::fromStdString(
            m_wsname)); // Pretty much just print out the string
    break;
  case numTSeries:
    if (filterStatus->isChecked())
      filter = 1;
    if (filterPeriod->isChecked())
      filter = 2;
    if (filterStatusPeriod->isChecked())
      filter = 3;
    m_mantidUI->importNumSeriesLog(QString::fromStdString(m_wsname),
                                   item->text(0), filter);
    break;
  case stringTSeries:
    m_mantidUI->importStrSeriesLog(item->text(0),
                                   item->data(0, Qt::UserRole).toString(),
                                   QString::fromStdString(m_wsname));
    break;
  case numericArray:
    logData = m_ei->getLog(item->text(0).toStdString());
    if (!logData)
      return;
    m_mantidUI->importString(
        item->text(0), QString::fromStdString(logData->value()),
        QString::fromStdString(","), QString::fromStdString(m_wsname));
    break;
  default:
    throw std::invalid_argument("Error importing log entry, wrong data type");
  }
}
