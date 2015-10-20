#include "MantidQtCustomDialogs/GetNegMuMuonicXRDDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtCustomDialogs/MantidGLWidget.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QLineEdit>
#include <QValidator>
#include <QFormLayout>

namespace MantidQt {
namespace CustomDialogs {
DECLARE_DIALOG(GetNegMuMuonicXRDDialog)

/**
 * Default constructor.
 * @param parent :: Parent dialog.
 */

GetNegMuMuonicXRDDialog::GetNegMuMuonicXRDDialog(QWidget *parent)
    : API::AlgorithmDialog(parent), m_periodicTable(NULL), m_yPosition(NULL),
      m_groupWorkspaceNameInput(NULL), m_showLegendCheck(NULL) {}

/// Initialise the layout
void GetNegMuMuonicXRDDialog::initLayout() {
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  this->setMaximumHeight(400);
  this->setMaximumWidth(675);
  // assign periodicTable member to a new periodicTable
  m_periodicTable = new PeriodicTableWidget();

  // assign m_yPosition member to a new QLineEdit
  m_yPosition = new QLineEdit();
  // assign GroupWorkspaceName member to a new QLineEdit
  m_groupWorkspaceNameInput = new QLineEdit();
  auto *groupWsInputLabel = new QLabel("OutputWorkspace");
  m_groupWorkspaceNameInput->setMaximumWidth(250);
  // Disable all buttons on the periodicTable
  // as we only have a select few that need to be
  // enabled.
  m_periodicTable->disableAllElementButtons();

  /*Elements Enabled Correspond to those for which we
  * have data for in the dictionary found in
  * GetNegMuMuonicXRD.py file
  */
  enableElementsForGetNegMuMuonicXRD();

  // main layout for the dialog (everything will be added to this)
  auto *main_layout = new QVBoxLayout(this);

  // run button for executing the algorithm
  auto *runButton = new QPushButton("Run");

  // label for the QLineEdit for m_yPosition property
  auto *m_yPositionLabel = new QLabel("Y Position");

  /*validator allows only numeric input for m_yPosition
   *this helps with validating the input.
   *Does not detect empty string as invalid input.
   */
  auto m_yPositionNumericValidator = new QDoubleValidator();

  // YPosition LineEdit Attributes
  m_yPosition->setMaximumWidth(250);
  m_yPosition->setPlaceholderText("-0.01");
  m_yPosition->setValidator(m_yPositionNumericValidator);

  // Run Button Attributes and signal/slot assignment
  runButton->setMaximumWidth(100);
  connect(runButton, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(validInput()), this, SLOT(accept()));

  // Show Legend button attributes and signal/slot asssignment
  m_showLegendCheck = new QCheckBox("Show Legend");
  connect(m_showLegendCheck, SIGNAL(clicked()), this, SLOT(showLegend()));
  // Adding Widgets to Layout
  main_layout->addWidget(m_periodicTable);
  main_layout->addWidget(m_showLegendCheck);
  main_layout->addWidget(m_yPositionLabel);
  main_layout->addWidget(m_yPosition);
  main_layout->addWidget(groupWsInputLabel);
  main_layout->addWidget(m_groupWorkspaceNameInput);
  main_layout->addWidget(runButton);
}

/**
 *
 */
void GetNegMuMuonicXRDDialog::showLegend() {
  bool checked = m_showLegendCheck->isChecked();
  m_periodicTable->showGroupLegend(checked);
}

/**
 * Enables the buttons for which we have data for in the GetNegMuMuonicXRD.py
 * dictionary of elements, by Periodic Table symbol.
 * i.e Au corresponds to Gold.
 */
void GetNegMuMuonicXRDDialog::enableElementsForGetNegMuMuonicXRD() {
  /* The GetNegMuMuonic algorithm only has data for these elements
   * The dictionary of elements and data can edited in the python file
   * for the algorithm, and the button for that element can be enabled
   * the same as the elements are below.
   */
  m_periodicTable->enableButtonByName("Au");
  m_periodicTable->enableButtonByName("Ag");
  m_periodicTable->enableButtonByName("Cu");
  m_periodicTable->enableButtonByName("Zn");
  m_periodicTable->enableButtonByName("Pb");
  m_periodicTable->enableButtonByName("As");
  m_periodicTable->enableButtonByName("Sn");
}

/**
 * Used for checking if the input is none empty for Y-Position Property
 * and if any elements have been selected from the periodicTableWidget
 * @param input :: A QString that is checked to see if it is empty.
*/

bool GetNegMuMuonicXRDDialog::validateDialogInput(QString input) {
  // empty check on input
  return (input != "");
}

/**
 * The Slot to gather input from the dialog, store it in the propertyValue
 * and then emit the signal for valid input. Preparing for accept() to be run.
*/
void GetNegMuMuonicXRDDialog::runClicked() {
  // getting a list of strings of elements selected from periodicTableWidget
  QString elementsSelectedStr = m_periodicTable->getAllCheckedElementsStr();
  // if no elements are selected from the PeriodicTableWidget, a pop-up appears
  // to the user.
  if (!validateDialogInput(elementsSelectedStr)) {
    QMessageBox::information(
        this, "GetNegMuMuonicXRDDialog",
        "No elements were selected, Please select an element from the table");
  }
  // If elements have been selected and y-position text is non-empty then
  // store the inputs as the corresponding propertyValues and emit validInput
  // signal.
  if (validateDialogInput(elementsSelectedStr)) {
    storePropertyValue("Elements", elementsSelectedStr);
    if (validateDialogInput(m_yPosition->text())) {
      storePropertyValue("YAxisPosition", m_yPosition->text());
    } else {
      // used as default value for m_yPosition property if the user does not
      // input
      // one.
      storePropertyValue("YAxisPosition", m_yPosition->placeholderText());
    }
    if (validateDialogInput(m_groupWorkspaceNameInput->text())) {
      storePropertyValue("OutputWorkspace", m_groupWorkspaceNameInput->text());
    }
    emit validInput();
  }
}
}
}
