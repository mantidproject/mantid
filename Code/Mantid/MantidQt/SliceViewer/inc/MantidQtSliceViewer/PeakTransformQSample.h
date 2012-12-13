#ifndef MANTID_SLICEVIEWER_PEAKTRANSFORMQSAMPLE_H_
#define MANTID_SLICEVIEWER_PEAKTRANSFORMQSAMPLE_H_

#include "MantidQtSliceViewer/PeakTransform.h"
#include "MantidQtSliceViewer/ConcretePeakTransformFactory.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    @class PeakTransformQSample
    Used to remap coordinates into a form consistent with an axis reordering.
    */
    class DLLExport PeakTransformQSample : public PeakTransform
    {
    public:
      PeakTransformQSample();
      PeakTransformQSample(const std::string& xPlotLabel, const std::string& yPlotLabel);
      virtual ~PeakTransformQSample();
      PeakTransformQSample(const PeakTransformQSample& other);
      PeakTransformQSample & operator=(const PeakTransformQSample & other);
      /// Virtual constructor
      PeakTransform_sptr clone() const;
      /// Transform peak.
      Mantid::Kernel::V3D transformPeak(const Mantid::API::IPeak& peak) const;
    };

    /// Typedef a factory for type of PeaksTransform.
    typedef ConcretePeakTransformFactory<PeakTransformQSample> PeakTransformQSampleFactory;

  }
}

#endif /* MANTID_SLICEVIEWER_PEAKTRANSFORMQSAMPLE_H_ */