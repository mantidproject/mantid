#include "MantidQtWidgets/MplCpp/Colorbar.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * Construct a colorbar based on the matplotlib colorbar using a linear
 * scale and default colormap
 * @param parent A pointer to its parent widget
 */
Colorbar::Colorbar(QWidget *parent) : QWidget(parent) {}

/**
 * @brief Legacy constructor to specify the scale type as an integer
 * @param type An integer (0=Linear, 1=Log10,2=Power)
 * @param parent A pointer to the parent widget
 */
Colorbar::Colorbar(int type, QWidget *parent) : QWidget(parent) {}

//void Colorbar::setupColorBarScaling(const MantidColorMap &)
//{

//}

//void Colorbar::setMinValue(double)
//{

//}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
