// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------
// Includes
//----------------------------------

#include "LabelToolLogValuesDialog.h"

// Mantid
#include "Mantid/MantidUI.h"
#include <LegendWidget.h>

// Qt
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

  setDialogWindowTitle(wsname);

  setTreeWidgetColumnNames();

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

  QVBoxLayout *hbox = new QVBoxLayout;
  addImportAndCloseButtonsTo(hbox);
  addExperimentInfoSelectorTo(hbox);

  // Finish laying out the right side
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

  setUpTreeWidgetConnections();
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
 *							fails
 *
 */
void LabelToolLogValuesDialog::importItem(QTreeWidgetItem *item) {

  // Dynamic cast up to LegendWidget, which is the class of the
  // one containing the label, in order to use setText
  auto parentWidget = dynamic_cast<LegendWidget *>(m_parentContainer);
  if (nullptr == parentWidget) { // if dynamic cast fails, don't fail silently
    throw std::bad_cast();
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
