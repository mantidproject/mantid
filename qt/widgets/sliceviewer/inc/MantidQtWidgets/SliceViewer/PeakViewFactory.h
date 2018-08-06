#ifndef MANTID_SLICEVIEWER_PEAK_VIEW_FACTORY_H_
#define MANTID_SLICEVIEWER_PEAK_VIEW_FACTORY_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidQtWidgets/SliceViewer/EllipsoidPlaneSliceCalculator.h"
#include "MantidQtWidgets/SliceViewer/NonOrthogonalAxis.h"
#include "MantidQtWidgets/SliceViewer/PeakOverlayViewFactoryBase.h"
#include "MantidQtWidgets/SliceViewer/PeakRepresentation.h"
#include "MantidQtWidgets/SliceViewer/PeakViewColor.h"

#include <QColor>

namespace MantidQt {
namespace SliceViewer {

/** PeakViewFactory : Creates an appropriate PeakView object

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at:
  <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class PeakViewFactory : public PeakOverlayViewFactoryBase {
public:
  PeakViewFactory(Mantid::API::IMDWorkspace_sptr mdWS,
                  Mantid::API::IPeaksWorkspace_sptr peaksWS, QwtPlot *plot,
                  QWidget *parent, const int plotXIndex, const int plotYIndex,
                  const size_t colorNumber = 0);
  virtual ~PeakViewFactory();
  boost::shared_ptr<PeakOverlayView> createView(
      PeaksPresenter *const presenter,
      Mantid::Geometry::PeakTransform_const_sptr transform) const override;
  void swapPeaksWorkspace(
      boost::shared_ptr<Mantid::API::IPeaksWorkspace> &peaksWS) override;
  void getNonOrthogonalInfo(NonOrthogonalAxis &info) override;

private:
  // Selector for the correct representation of a single peak
  PeakRepresentation_sptr createSinglePeakRepresentation(
      const Mantid::Geometry::IPeak &peak, Mantid::Kernel::V3D position,
      Mantid::Geometry::PeakTransform_const_sptr transform) const;

  // Creates a cross-like representation
  PeakRepresentation_sptr createPeakRepresentationCross(
      Mantid::Kernel::V3D position,
      Mantid::Geometry::PeakTransform_const_sptr transform) const;

  // Creates a spherical representation
  PeakRepresentation_sptr
  createPeakRepresentationSphere(Mantid::Kernel::V3D position,
                                 const Mantid::Geometry::IPeak &peak) const;

  // Creates a spherical representation
  PeakRepresentation_sptr
  createPeakRepresentationEllipsoid(Mantid::Kernel::V3D position,
                                    const Mantid::Geometry::IPeak &peak) const;

  // Set color palette
  void setForegroundAndBackgroundColors(const size_t colourNumber);

  // The actual workspace
  Mantid::API::IMDWorkspace_sptr m_mdWS;

  /// Peaks workspace.
  Mantid::API::IPeaksWorkspace_sptr m_peaksWS;

  /// Color foreground
  PeakViewColor m_foregroundColor;

  /// Color background
  PeakViewColor m_backgroundColor;

  /// Ellipsoid calculator -- as we don't paint in parallel this is safe to
  /// share between the peaks
  std::shared_ptr<Mantid::SliceViewer::EllipsoidPlaneSliceCalculator>
      m_calculator;
};
} // namespace SliceViewer
} // namespace MantidQt

#endif
