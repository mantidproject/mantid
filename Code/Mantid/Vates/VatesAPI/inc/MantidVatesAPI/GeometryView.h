#ifndef GEOMETRY_VIEW_H
#define GEOMETRY_VIEW_H
#include "MantidKernel/System.h"
#include "MantidVatesAPI/DimensionView.h"
#include "MantidVatesAPI/DimensionViewFactory.h"
namespace Mantid
{
  namespace VATES
  {
    class QString;
    class DLLExport GeometryView 
    {
    public:
      virtual void addDimensionView(DimensionView*) = 0;
      virtual std::string getGeometryXMLString() const = 0;
      virtual const DimensionViewFactory& getDimensionViewFactory() = 0;
      virtual ~GeometryView()=0{};
      virtual void raiseModified() = 0;
      virtual void raiseNoClipping() = 0;
    };
  }
}

#endif