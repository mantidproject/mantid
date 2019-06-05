// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------
// Includes
//----------------------------------
#include "MantidSampleLogDialog.h"

// Mantid
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidUI.h"

// Qt
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>

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
  setDialogWindowTitle(wsname);

  setTreeWidgetColumnNames();

  QHBoxLayout *uiLayout = new QHBoxLayout;
  uiLayout->addWidget(m_tree);

  // ----- Filtering options --------------
  QGroupBox *groupBox = new QGroupBox(tr("Filter log values by"));

  filterNone = new QRadioButton("None");
  filterStatus = new QRadioButton("Status");
  filterPeriod = new QRadioButton("Period");
  filterStatusPeriod = new QRadioButton("Status + Period");
  filterStatusPeriod->setChecked(true);
  const std::vector<QRadioButton *> filterRadioButtons{
      filterNone, filterStatus, filterPeriod, filterStatusPeriod};

  // Add options to layout
  QVBoxLayout *vbox = new QVBoxLayout;
  for (auto *radioButton : filterRadioButtons) {
    vbox->addWidget(radioButton);
  }
  groupBox->setLayout(vbox);

  // Changing filter option updates stats
  for (auto *radioButton : filterRadioButtons) {
    connect(radioButton, SIGNAL(toggled(bool)), this,
            SLOT(showLogStatistics()));
  }

  // -------------- Statistics on logs ------------------------
  std::string stats[NUM_STATS] = {
      "Min:",     "Max:",      "Mean:",         "Median:",
      "Std Dev:", "Time Avg:", "Time Std Dev:", "Duration:"};
  QGroupBox *statsBox = new QGroupBox("Log Statistics");
  QFormLayout *statsBoxLayout = new QFormLayout;
  for (size_t i = 0; i < NUM_STATS; i++) {
    statLabels[i] = new QLabel(stats[i].c_str());
    statValues[i] = new QLineEdit("");
    statValues[i]->setReadOnly(true);
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

  setUpTreeWidgetConnections();
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
  Mantid::Kernel::Property *logData = nullptr;
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

/**
 * Return filter type based on which radio button is selected
 * @returns :: Filter type selected in UI
 */
Mantid::API::LogFilterGenerator::FilterType
MantidSampleLogDialog::getFilterType() const {
  if (filterStatus->isChecked()) {
    return Mantid::API::LogFilterGenerator::FilterType::Status;
  } else if (filterPeriod->isChecked()) {
    return Mantid::API::LogFilterGenerator::FilterType::Period;
  } else if (filterStatusPeriod->isChecked()) {
    return Mantid::API::LogFilterGenerator::FilterType::StatusAndPeriod;
  } else {
    return Mantid::API::LogFilterGenerator::FilterType::None;
  }
}
