//----------------------------------
// Includes
//----------------------------------

#include "LabelToolLogValuesDialog.h"

// STD
#include <sstream>

// Mantid
#include <LegendWidget.h>
#include <Mantid/MantidUI.h>
#include <MantidAPI/MultipleExperimentInfos.h>

// Qt
#include <QHeaderView>
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>

//----------------------------------
// Namespaces
//----------------------------------

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

//----------------------------------
// Public methods
//----------------------------------
/**
* Construct an object of this type
* @param wsname :: The name of the workspace object from which to retrieve the
* log files
* @param parentContainer :: The widget that is the container this dialog
* @param flags :: Window flags that are passed the the QDialog constructor
* @param experimentInfoIndex :: optional index in the array of
*        ExperimentInfo objects. Should only be non-zero for MDWorkspaces.
*/
LabelToolLogValuesDialog::LabelToolLogValuesDialog(const QString &wsname,
                                                   QWidget *parentContainer,
                                                   Qt::WFlags flags,
                                                   size_t experimentInfoIndex)
    : SampleLogDialogBase(wsname, parentContainer, flags, experimentInfoIndex) {

  std::stringstream ss;
  ss << "MantidPlot - " << wsname.toStdString().c_str() << " sample logs";
  setWindowTitle(QString::fromStdString(ss.str()));

  QStringList titles;
  titles << "Name"
         << "Type"
         << "Value"
         << "Units";
  m_tree->setHeaderLabels(titles);
  m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
  QHeaderView *hHeader = (QHeaderView *)m_tree->header();
  hHeader->setResizeMode(2, QHeaderView::Stretch);
  hHeader->setStretchLastSection(false);

  QHBoxLayout *uiLayout = new QHBoxLayout;
  uiLayout->addWidget(m_tree);

  // -------------- Statistics on logs ------------------------
  std::string stats[NUM_STATS] = {
      "Min:", "Max:", "Mean:", "Time Avg:", "Median:", "Std Dev:", "Duration:"};
  QGroupBox *statsBox = new QGroupBox("Log Statistics");
  QFormLayout *statsBoxLayout = new QFormLayout;

  for (size_t i = 0; i < NUM_STATS; i++) {
    statRadioChoice[i] = new QRadioButton(stats[i].c_str());
    statValues[i] = new QLineEdit("");
    statValues[i]->setReadOnly(true);

    statsBoxLayout->addRow(statRadioChoice[i], statValues[i]);
  }
  // Set default checked radio button
  statRadioChoice[0]->setChecked(true);
  statsBox->setLayout(statsBoxLayout);

  // -------------- The Import/Close buttons ------------------------
  QHBoxLayout *topButtons = new QHBoxLayout;
  buttonPlot = new QPushButton(tr("&Import selected log"));
  buttonPlot->setAutoDefault(true);
  buttonPlot->setToolTip(
      "Import log file as a table and construct a 1D graph if appropriate");
  topButtons->addWidget(buttonPlot);

  buttonClose = new QPushButton(tr("Close"));
  buttonClose->setToolTip("Close dialog");
  topButtons->addWidget(buttonClose);

  QVBoxLayout *hbox = new QVBoxLayout;

  // -------------- The ExperimentInfo selector------------------------
  boost::shared_ptr<Mantid::API::MultipleExperimentInfos> mei =
      AnalysisDataService::Instance().retrieveWS<MultipleExperimentInfos>(
          m_wsname);

  if (mei) {
    if (mei->getNumExperimentInfo() > 0) {
      QHBoxLayout *numSelectorLayout = new QHBoxLayout;
      QLabel *lbl = new QLabel("Experiment Info #");
      m_spinNumber = new QSpinBox;
      m_spinNumber->setMinimum(0);
      m_spinNumber->setMaximum(int(mei->getNumExperimentInfo()) - 1);
      m_spinNumber->setValue(int(m_experimentInfoIndex));
      numSelectorLayout->addWidget(lbl);
      numSelectorLayout->addWidget(m_spinNumber);
      // Double-click imports a log file
      connect(m_spinNumber, SIGNAL(valueChanged(int)), this,
              SLOT(selectExpInfoNumber(int)));
      hbox->addLayout(numSelectorLayout);
    }
  }

  // Finish laying out the right side
  hbox->addLayout(topButtons);
  hbox->addWidget(statsBox);
  hbox->addStretch(1);

  //--- Main layout With 2 sides -----
  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->addLayout(uiLayout, 1); // the tree
  mainLayout->addLayout(hbox, 0);
  // mainLayout->addLayout(bottomButtons);
  this->setLayout(mainLayout);

  // Call initialisation from base class
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

//------------------------------------------------------------------------------------------------
LabelToolLogValuesDialog::~LabelToolLogValuesDialog() {}

//------------------------------------------------------------------------------------------------
/** Changes the LabelWidget parent object by using the setText
* method and constructing a label based on the selected log
* and value or generated statistics, and then calls close()
* inherited from the parent widget after importing the label.
*
* This is intentional because importing multiple labels will
* place them on the same spot and it can get unreadable.
*
* The parent container variable is dynamically cast up to LegendWidget.
*
*	@param item :: The currently selected item from the log list
*	@throws std::bad_cast :: The exception is throws if the dynamic_cast
*fails
*
*/
void LabelToolLogValuesDialog::importItem(QTreeWidgetItem *item) {

  // Dynamic cast up to LegendWidget, which is the class of the
  // one containing the label, in order to use setText
  auto parentWidget = dynamic_cast<LegendWidget *>(m_parentContainer);
  if (parentWidget == NULL) { // if dynamic cast fails, don't fail silently
    throw new std::bad_cast;
  }

  // find which radio box is checked
  size_t activeRadioBoxPosition = 0;
  for (size_t i = 0; i < NUM_STATS; ++i) {
    if (statRadioChoice[i]->isChecked()) {
      activeRadioBoxPosition = i;
      break; // once found stop looking
    }
  }

  // Container for the full log description,
  // starts with the name of the item from the tree
  QString logValuesString = item->text(0) + '\n';

  // Get the text from the selected stat value to determine
  // if the item has any statistics calculated for it
  // It's in a local variable to avoid repetition
  QString statValuesText = statValues[activeRadioBoxPosition]->text();

  // NOTE:
  // If the log has no log statistics, import value field(2) and units(3)
  if (statValuesText.isEmpty()) {
    logValuesString += "Value: " + item->text(2) + " " + item->text(3);
  } else {
    // else import the selected statistic
    // constructing the log string using the hardcoded fields
    logValuesString +=
        statRadioChoice[activeRadioBoxPosition]->text() + "  " + statValuesText;
  }

  parentWidget->setText(logValuesString);
  close();
}
