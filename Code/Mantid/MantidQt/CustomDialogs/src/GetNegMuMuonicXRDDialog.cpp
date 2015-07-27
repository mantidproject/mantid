#include "MantidQtCustomDialogs/GetNegMuMuonicXRDDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtMantidWidgets/PeriodicTableWidget.h"

namespace MantidQt{
  namespace CustomDialogs{
    DECLARE_DIALOG(GetNegMuMuonicXRDDialog)

    GetNegMuMuonicXRDDialog::GetNegMuMuonicXRDDialog(QWidget *parent):API::AlgorithmDialog(parent)
    {
    }

    void GetNegMuMuonicXRDDialog::initLayout()
    {
      auto *main_layout = new QVBoxLayout(this);  
      auto *p = new PeriodicTableWidget();
      main_layout->addWidget(p);
    }
  }
}