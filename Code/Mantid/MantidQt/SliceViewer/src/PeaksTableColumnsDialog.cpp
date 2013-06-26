#include "MantidQtSliceViewer/PeaksTableColumnsDialog.h"
#include "ui_PeaksTableColumnsDialog.h"

PeaksTableColumnsDialog::PeaksTableColumnsDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::PeaksTableColumnsDialog)
{
  ui->setupUi(this);
}

PeaksTableColumnsDialog::~PeaksTableColumnsDialog()
{
  delete ui;
}
