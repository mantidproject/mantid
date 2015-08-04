#include "MantidQtCustomDialogs/GetNegMuMuonicXRDDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtMantidWidgets/PeriodicTableWidget.h"
#include "MantidQtCustomDialogs/MantidGLWidget.h"
#include <QMessageBox>
#include <QLineEdit>
#include <QValidator>
#include <QFormLayout>

namespace MantidQt {
namespace CustomDialogs {
DECLARE_DIALOG(GetNegMuMuonicXRDDialog)

auto *periodicTable = new PeriodicTableWidget();
auto *yPosition = new QLineEdit();
bool selectElementsIsClicked;

GetNegMuMuonicXRDDialog::GetNegMuMuonicXRDDialog(QWidget *parent)
    : API::AlgorithmDialog(parent) {
}


void GetNegMuMuonicXRDDialog::initLayout() {
  auto *main_layout = new QVBoxLayout(this);
  auto *button_layout = new QFormLayout(this);
  //auto *SelectElements = new QPushButton("Select Elements");
  //SelectElements->setMaximumWidth(100);
 // connect(SelectElements, SIGNAL(clicked()), this, SLOT(SelectElementsClicked()));
 // selectElementsIsClicked = false;
  yPosition->setMaximumWidth(100);
  auto *yPositionLabel = new QLabel("Y Position");
  storePropertyValue("YAxisPosition", yPosition->text());
  auto yPositionNumericValidator = new QDoubleValidator();
  yPosition->setMaxLength(10);
  yPosition->setValidator(yPositionNumericValidator);
  auto *runButton = new QPushButton("Run");
  runButton->setMaximumWidth(100);
  connect(runButton, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(runButton, SIGNAL(clicked()), this, SLOT(accept()));
  main_layout->addWidget(periodicTable);
  //button_layout->addWidget(SelectElements);
  button_layout->addWidget(yPositionLabel);
  button_layout->addWidget(yPosition);
  button_layout->addWidget(runButton);
  main_layout->addLayout(button_layout);
}

QString getElementsSelectedFromPeriodicTable() {
  return periodicTable->getAllCheckedElementsStr();
}
/*
void GetNegMuMuonicXRDDialog::SelectElementsClicked() {
  selectElementsIsClicked = true;
}
*/
void GetNegMuMuonicXRDDialog::runClicked(){
  if (yPosition->text() == ""){
      QMessageBox::information(
          this, "GetNegMuMuonicXRDDialog",
          "No Y Axis Position was specified, please enter a value for Y Axis Position");
      return;
  }
  QString elementsSelectedStr = getElementsSelectedFromPeriodicTable();
    if (elementsSelectedStr == "") {
      QMessageBox::information(
          this, "GetNegMuMuonicXRDDialog",
          "No elements were selected, Please select an element from the table");
      return;
    }
    storePropertyValue("Elements", elementsSelectedStr);
    std::cout << elementsSelectedStr.toStdString();
  storePropertyValue("YAxisPosition", yPosition->text());
}

/*void GetNegMuMuonicXRDDialog::parseInput() {
  if (selectElementsIsClicked) {
    QString elementsSelectedStr = getElementsSelectedFromPeriodicTable();
    if (elementsSelectedStr == "") {
      QMessageBox::information(
          this, "GetNegMuMuonicXRDDialog",
          "No elements were selected, Please select an element from the table");
      return;
    }
    storePropertyValue("Elements", elementsSelectedStr);
    std::cout << elementsSelectedStr.toStdString();
  }
}*/
}
}
