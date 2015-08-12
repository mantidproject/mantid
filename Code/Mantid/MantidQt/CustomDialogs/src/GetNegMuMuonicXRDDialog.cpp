#include "MantidQtCustomDialogs/GetNegMuMuonicXRDDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtCustomDialogs/MantidGLWidget.h"
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
    : API::AlgorithmDialog(parent) {}

///Initialise the layout
void GetNegMuMuonicXRDDialog::initLayout() {
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  this->setMaximumHeight(400);
  this->setMaximumWidth(675);
  //assign periodicTable member to a new periodicTable
  periodicTable = new PeriodicTableWidget();

  //assign yPosition member to a new QLineEdit
  yPosition = new QLineEdit();

  //Disable all buttons on the periodicTable
  //as we only have a select few that need to be
  //enabled.
  periodicTable->disableAllElementButtons();

  /*Elements Enabled Correspond to those for which we
  * have data for in the dictionary found in
  * GetNegMuMuonicXRD.py file
  */
  enableElementsForGetNegMuMuonicXRD();

  //main layout for the dialog (everything will be added to this)
  auto *main_layout = new QVBoxLayout(this);

  // run button for executing the algorithm
  auto *runButton = new QPushButton("Run");

  //label for the QLineEdit for yPosition property
  auto *yPositionLabel = new QLabel("Y Position");

  /*validator allows only numeric input for yPosition
   *this helps with validating the input.
   *Does not detect empty string as invalid input.
   */
  auto yPositionNumericValidator = new QDoubleValidator();

  //YPosition LineEdit Attributes
  yPosition->setMaximumWidth(250);
  yPosition->setPlaceholderText("-0.01");
  yPosition->setValidator(yPositionNumericValidator);

  //Run Button Attributes and signal/slot assignment
  runButton->setMaximumWidth(100);
  connect(runButton, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(this, SIGNAL(validInput()), this, SLOT(accept()));

  //Adding Widgets to Layout
  main_layout->addWidget(periodicTable);
  main_layout->addWidget(yPositionLabel);
  main_layout->addWidget(yPosition);
  main_layout->addWidget(runButton);
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
  periodicTable->enableButtonByName("Au");
  periodicTable->enableButtonByName("Ag");
  periodicTable->enableButtonByName("Cu");
  periodicTable->enableButtonByName("Zn");
  periodicTable->enableButtonByName("Pb");
  periodicTable->enableButtonByName("As");
  periodicTable->enableButtonByName("Sn");
}

/**
 * Used for checking if the input is none empty for Y-Position Property
 * and if any elements have been selected from the periodicTableWidget
 * @param input :: A QString that is checked to see if it is empty.
*/

bool GetNegMuMuonicXRDDialog::validateDialogInput(QString input) {
  //empty check on input
  return (input != "");
}

/**
 * The Slot to gather input from the dialog, store it in the propertyValue
 * and then emit the signal for valid input. Preparing for accept() to be run.
*/
void GetNegMuMuonicXRDDialog::runClicked() {
  //if y-position text is empty, a pop-up appears to the user.
  if (!validateDialogInput(yPosition->text())) {
    QMessageBox::information(this, "GetNegMuMuonicXRDDialog",
                             "No Y Axis Position was specified, please enter a "
                             "value or run with default value of -0.001");
  }
  //getting a list of strings of elements selected from periodicTableWidget
  QString elementsSelectedStr = periodicTable->getAllCheckedElementsStr();
  //if no elements are selected from the PeriodicTableWidget, a pop-up appears to the user.
  if (!validateDialogInput(elementsSelectedStr)) {
    QMessageBox::information(
        this, "GetNegMuMuonicXRDDialog",
        "No elements were selected, Please select an element from the table");
  }
  // If elements have been selected and y-position text is non-empty then
  // store the inputs as the corresponding propertyValues and emit validInput signal.
  if (validateDialogInput(elementsSelectedStr) &&
      validateDialogInput(yPosition->text())) {
    storePropertyValue("Elements", elementsSelectedStr);
    storePropertyValue("YAxisPosition", yPosition->text());
    emit validInput();
  }
  //used as default value for yPosition property if the user does not input one.
  yPosition->setText("-0.001");
}
}
}
