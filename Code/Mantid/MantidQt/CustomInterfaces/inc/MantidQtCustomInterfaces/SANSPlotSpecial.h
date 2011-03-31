#include <QFrame>
#include "ui_SANSPlotSpecial.h"

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
