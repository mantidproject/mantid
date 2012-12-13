#ifndef MANTID_SLICEVIEWER_PeakTransformQLab_H_
#define MANTID_SLICEVIEWER_PeakTransformQLab_H_

#include "MantidQtSliceViewer/PeakTransform.h"
#include "MantidQtSliceViewer/ConcretePeakTransformFactory.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    @class PeakTransformQLab
    Used to remap coordinates into a form consistent with an axis reordering.
    */
    class DLLExport PeakTransformQLab : public PeakTransform
    {
    public:
      PeakTransformQLab();
      PeakTransformQLab(const std::string& xPlotLabel, const std::string& yPlotLabel);
      virtual ~PeakTransformQLab();
      PeakTransformQLab(const PeakTransformQLab& other);
      PeakTransformQLab & operator=(const PeakTransformQLab & other);
      /// Virtual constructor
      PeakTransform_sptr clone() const;
      /// Transform peak.
      Mantid::Kernel::V3D transformPeak(const Mantid::API::IPeak& peak) const;
    };

    /// Typedef a factory for type of PeaksTransform.
    typedef ConcretePeakTransformFactory<PeakTransformQLab> PeakTransformQLabFactory;

  }
}

#endif /* MANTID_SLICEVIEWER_PeakTransformQLab_H_ */