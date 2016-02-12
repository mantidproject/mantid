#ifndef MANTID_SLICEVIEWER_PEAK_VIEW_FACTORY_H_
#define MANTID_SLICEVIEWER_PEAK_VIEW_FACTORY_H_

#include "MantidQtSliceViewer/PeakOverlayViewFactoryBase.h"
#include "MantidQtSliceViewer/PeakRepresentation.h"
#include "MantidQtSliceViewer/PeakViewColor.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"

#include <QColor>

namespace MantidQt
{
namespace SliceViewer
{


class PeakViewFactory : public PeakOverlayViewFactoryBase
{
public:
    PeakViewFactory(Mantid::API::IMDWorkspace_sptr mdWS,
                    Mantid::API::IPeaksWorkspace_sptr peaksWS, QwtPlot *plot,
                    QWidget *parent, const int plotXIndex, const int plotYIndex,
                    const size_t colorNumber = 0);
    virtual ~PeakViewFactory();
    boost::shared_ptr<PeakOverlayView> createView(
        PeaksPresenter *const presenter,
        Mantid::Geometry::PeakTransform_const_sptr transform) const override;
    int FOM() const override;
    void swapPeaksWorkspace(
        boost::shared_ptr<Mantid::API::IPeaksWorkspace> &peaksWS) override;

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
    PeakRepresentation_sptr createPeakRepresentationEllipsoid(
        Mantid::Kernel::V3D position,
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
};
}
}

#endif
