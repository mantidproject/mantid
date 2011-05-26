#ifndef _DIMENSION_VIEW_FACTORY_H
#define _DIMENSION_VIEW_FACTORY_H

#include "MantidVatesAPI/DimensionView.h"
namespace Mantid
{
  namespace VATES
  {
    class DLLExport DimensionViewFactory
    {
    public:
      virtual DimensionView* create() const = 0;
      virtual ~DimensionViewFactory()=0{}
    };
  }
}

#endif