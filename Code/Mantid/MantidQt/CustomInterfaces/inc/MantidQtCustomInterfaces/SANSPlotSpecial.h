#ifndef MANTIDQTCUSTOMINTERFACES_SANS_DISPLAY_TAB
#define MANTIDQTCUSTOMINTERFACES_SANS_DISPLAY_TAB

#include <QFrame>
#include "ui_SANSPlotSpecial.h"
#include "MantidQtAPI/UserSubWindow.h"

namespace MantidQt
{
namespace CustomInterfaces
{

class SANSPlotSpecial : public QFrame
{
  Q_OBJECT

public:
  SANSPlotSpecial(QWidget *parent = 0);

private:
  Ui::SANSPlotSpecial m_uiForm;

};

}
}

#endif