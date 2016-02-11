#include "MantidQtSliceViewer/PeakPalette.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace MantidQt
{
namespace SliceViewer
{
template <> PeakPalette<QColor>::PeakPalette()
{
    m_foregroundMap = {{0, {"#bf7651"}},
                       {1, {"#bd97cb"}},
                       {2, {"#ceeeea"}},
                       {3, {"#da4a52"}},
                       {4, {"#9bc888"}},
                       {5, {"#ffe181"}},
                       {6, {"#e8b7c1"}},
                       {7, {"#f38235"}},
                       {8, {"#8390c6"}},
                       {9, {"#4ca0ac"}}};
    m_backgroundMap = {{0, {"#bf7651"}},
                       {1, {"#bd97cb"}},
                       {2, {"#ceeeea"}},
                       {3, {"#da4a52"}},
                       {4, {"#9bc888"}},
                       {5, {"#ffe181"}},
                       {6, {"#e8b7c1"}},
                       {7, {"#f38235"}},
                       {8, {"#8390c6"}},
                       {9, {"#4ca0ac"}}};
}
}
}
