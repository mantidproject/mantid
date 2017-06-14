#include "preprocesswindow.h"
#include "ui_preprocesswindow.h"

PreprocessWindow::PreprocessWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::PreprocessWindow)
{
  ui->setupUi(this);
}

PreprocessWindow::~PreprocessWindow()
{
  delete ui;
}
