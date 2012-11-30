#ifndef MANTID_SLICEVIEWER_PEAKTRANSFORMHKL_H_
#define MANTID_SLICEVIEWER_PEAKTRANSFORMHKL_H_

#include "MantidQtSliceViewer/PeakTransform.h"

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
      PeakTransformHKL(const std::string& xPlotLabel, const std::string& yPlotLabel);
      virtual ~PeakTransformHKL();
      PeakTransformHKL(const PeakTransformHKL& other);
      PeakTransformHKL & operator=(const PeakTransformHKL & other);
      /// Virtual constructor
      PeakTransform_sptr clone() const;
    };

  }
}

#endif /* MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTERHKL_H_ */