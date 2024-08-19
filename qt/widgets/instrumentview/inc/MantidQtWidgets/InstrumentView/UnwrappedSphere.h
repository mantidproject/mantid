// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "RotationSurface.h"

namespace MantidQt {
namespace MantidWidgets {

/**
 * Implementation of UnwrappedSurface as a cylinder
 */
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW UnwrappedSphere : public RotationSurface {
public:
  UnwrappedSphere(const InstrumentActor *rootActor, const Mantid::Kernel::V3D &origin, const Mantid::Kernel::V3D &axis,
                  const QSize &widgetSize, const bool maintainAspectRatio);

protected:
  void rotate(const UnwrappedDetector &udet, Mantid::Kernel::Quat &R) const override;
  void project(const size_t detIndex, double &u, double &v, double &uscale, double &vscale) const override;
  void project(const Mantid::Kernel::V3D &position, double &u, double &v, double &uscale,
               double &vscale) const override;
};

} // namespace MantidWidgets
} // namespace MantidQt
