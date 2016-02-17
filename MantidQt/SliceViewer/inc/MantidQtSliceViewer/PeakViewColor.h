#ifndef MANTID_SLICEVIEWER_PEAK_VIEW_COLOR_H_
#define MANTID_SLICEVIEWER_PEAK_VIEW_COLOR_H_

#include <QColor>

struct PeakViewColor {
    PeakViewColor(QColor colorCross = QColor(), QColor colorSphere = QColor(), QColor colorEllipsoid = QColor())
        : colorCross(colorCross), colorSphere(colorSphere),
          colorEllipsoid(colorEllipsoid) { }

    bool operator==(const PeakViewColor &other) {
      auto sameColorCross = this->colorCross == other.colorCross;
      auto sameColorSphere = this->colorSphere == other.colorSphere;
      auto sameColorEllipsoid = this->colorEllipsoid == other.colorEllipsoid;

      return sameColorCross && sameColorSphere && sameColorEllipsoid;
    }

    bool operator!=(const PeakViewColor &other) {
      return !(*this == other);
    }

    QColor colorCross;
    QColor colorSphere;
    QColor colorEllipsoid;
};

#endif
