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
    class DLLExport ConcretePeakTransformFactory : public PeakTransformFactory
    {
    public:

      /**
      Constructor
      */
      ConcretePeakTransformFactory() 
      {
      }

      /**
      Overriden Factory Method.
      @param xPlotLabel : X-axis plot label
      @param yPlotLable : Y-axis plot label
      */
      virtual PeakTransform_sptr createTransform(const std::string& xPlotLabel, const std::string& yPlotLabel) const
      {
        return boost::make_shared<PeakTransformProduct>(xPlotLabel, yPlotLabel);
      }

      /**
      Overriden Factory Method.
      */
      virtual PeakTransform_sptr createDefaultTransform() const
      {
        return boost::make_shared<PeakTransformProduct>();
      }

      /// Destructor
      virtual ~ConcretePeakTransformFactory(){}
    };
  }
}

#endif