#ifndef MANTID_SLICEVIEWER_CONCRETEPEAKTRANSFORMFACTORY_H_
#define MANTID_SLICEVIEWER_CONCRETEPEAKTRANSFORMFACTORY_H_

#include "MantidQtSliceViewer/PeakTransformFactory.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    @class ConcretePeakTransformFactory
    Concrete PeakTransformFactory producing PeakTransforms of type provided by type arguement
    */
    template <typename PeakTransformProduct>
    class DLLExport ConcretePeakTransformFactory
    {
    public:

      /**
      Overriden Factory Method.
      */
      virtual PeakTransform_sptr createTransform(const std::string& xPlotLabel, const std::string& yPlotLabel)
      {
        return boost::make_shared<PeakTransformProduct>(xPlotLabel, yPlotLabel);
      }

      /// Destructor
      virtual ~ConcretePeakTransformFactory(){}
    };
  }
}

#endif