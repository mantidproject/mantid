#include "MantidQtCustomDialogs/MuonicXRDDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtMantidWidgets/PeriodicTableWidget.h"

namespace MantidQt{
  namespace CustomDialogs{

    MuonicXRDDialog:: MuonicXRDDialog(QWidget *parent):API::AlgorithmDialog(parent)
    {
    }

    void MuonicXRDDialog::initLayout()
    {
      auto *main_layout = new QVBoxLayout(this);  
      auto *p = new PeriodicTableWidget();
      main_layout->addWidget(p);
    }
  }
}