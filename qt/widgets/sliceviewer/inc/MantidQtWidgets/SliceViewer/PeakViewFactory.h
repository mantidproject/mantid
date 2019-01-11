// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
