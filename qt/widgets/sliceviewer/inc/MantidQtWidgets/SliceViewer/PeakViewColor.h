#ifndef MANTID_SLICEVIEWER_PEAK_VIEW_COLOR_H_
#define MANTID_SLICEVIEWER_PEAK_VIEW_COLOR_H_

#include <QColor>

/**
 * The PeakViewColor struct holds the color inforamtion for the different peak
 *marker types.
 * Currently there is a color for:
 * 1. the cross representation which is being displayed for non-shape peak types
 *(colorCross)
 * 2. the circle representation which is being displayed for spherical peak
 *types (colorSphere)
 * 3. the ellipse representation which is being displayed for ellipsoidal peak
 *types (colorEllipsoid)
 *
 * New peak types will need to have a color entry registered here.
 */
struct PeakViewColor {
  PeakViewColor(QColor colorCross = QColor(), QColor colorSphere = QColor(),
                QColor colorEllipsoid = QColor())
      : colorCross(colorCross), colorSphere(colorSphere),
        colorEllipsoid(colorEllipsoid) {}

  bool operator==(const PeakViewColor &other) const {
    auto sameColorCross = this->colorCross == other.colorCross;
    auto sameColorSphere = this->colorSphere == other.colorSphere;
    auto sameColorEllipsoid = this->colorEllipsoid == other.colorEllipsoid;

    return sameColorCross && sameColorSphere && sameColorEllipsoid;
  }

  bool operator!=(const PeakViewColor &other) const {
    return !(*this == other);
  }

  QColor colorCross;
  QColor colorSphere;
  QColor colorEllipsoid;
};

#endif
