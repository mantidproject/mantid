#include "MantidQtSliceViewer/PeakOverlayViewFactorySelector.h"

namespace MantidQt
{
  namespace SliceViewer
  {
      PeakOverlayViewFactorySelector::PeakOverlayViewFactorySelector()
      {

      }

      PeakOverlayViewFactorySelector::~PeakOverlayViewFactorySelector()
      {

      }

      void PeakOverlayViewFactorySelector::registerCandidate(PeakOverlayViewFactory_sptr factory)
      {
        throw std::runtime_error("Not implemented");
      }

      PeakOverlayViewFactory_sptr PeakOverlayViewFactorySelector::makeSelection() const
      {
        throw std::runtime_error("Not implemented");
      }

  }
}
