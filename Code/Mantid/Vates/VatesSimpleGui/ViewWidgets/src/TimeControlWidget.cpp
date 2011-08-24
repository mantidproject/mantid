#include "MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h"

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

TimeControlWidget::TimeControlWidget(QWidget *parent) : QWidget(parent) {
    this->ui.setupUi(this);
}

TimeControlWidget::~TimeControlWidget() {
}

}
}
}
