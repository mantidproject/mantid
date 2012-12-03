#ifndef MANTID_SLICEVIEWER_PEAKTRANSFORMHKL_H_
#define MANTID_SLICEVIEWER_PEAKTRANSFORMHKL_H_

#include "MantidQtSliceViewer/PeakTransform.h"
#include "MantidQtSliceViewer/ConcretePeakTransformFactory.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    @class PeakTransformHKL
    Used to remap coordinates into a form consistent with an axis reordering.
    */
    class DLLExport PeakTransformHKL : public PeakTransform
    {
    public:
      PeakTransformHKL();
      PeakTransformHKL(const std::string& xPlotLabel, const std::string& yPlotLabel);
      virtual ~PeakTransformHKL();
      PeakTransformHKL(const PeakTransformHKL& other);
      PeakTransformHKL & operator=(const PeakTransformHKL & other);
      /// Virtual constructor
      PeakTransform_sptr clone() const;
      /// Transform peak.
      Mantid::Kernel::V3D transformPeak(const Mantid::API::IPeak& peak) const;
    };

    /// Typedef a factory for type of PeaksTransform.
    typedef ConcretePeakTransformFactory<PeakTransformHKL> PeakTransformHKLFactory;

  }
}

#endif /* MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTERHKL_H_ */