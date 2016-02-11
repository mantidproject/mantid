#ifndef MANTID_SLICEVIEWER_PEAK_VIEW_PALETTE_H_
#define MANTID_SLICEVIEWER_PEAK_VIEW_PALETTE_H_

#include <QColor>

struct PeakViewColor {
    PeakViewColor(QColor colorCross = QColor(), QColor colorSphere = QColor(), QColor colorEllipsoid = QColor())
        : colorCross(colorCross), colorSphere(colorSphere),
          colorEllipsoid(colorEllipsoid) { }
    QColor colorCross;
    QColor colorSphere;
    QColor colorEllipsoid;
};



#endif
