#ifndef MANTID_SLICEVIEWER_PEAKTRANSFORMFACTORY_H_
#define MANTID_SLICEVIEWER_PEAKTRANSFORMFACTORY_H_

#include "MantidQtSliceViewer/PeakTransform.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    @class PeakTransformFactory
    Abstract type defining Factory Method interface for generating PeakTransforms
    */
    class DLLExport PeakTransformFactory
    {
    public:
      virtual PeakTransform_sptr createTransform(const std::string& xPlotLabel, const std::string& yPlotLabel) = 0;
      virtual ~PeakTransformFactory(){}
    };
  }
}

#endif