#ifndef DIMENSION_VIEW_H_
#define DIMENSION_VIEW_H_
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
namespace Mantid
{
  namespace VATES
  {
    class DimensionPresenter;
    class DLLExport DimensionView
    {
    public:
      virtual void configure() = 0;
      virtual void showAsNotIntegrated(Mantid::Geometry::VecIMDDimension_sptr nonIntegratedDims) = 0;
      virtual void showAsIntegrated() = 0;
      virtual void accept(DimensionPresenter* pDimensionPresenter) = 0; //TODO should accept non-deleting unique pointer.
      virtual std::string getDimensionId() const = 0;
      virtual double getMaximum() const = 0;
      virtual double getMinimum() const = 0;
      virtual unsigned int getNBins() const = 0;
      virtual unsigned int getSelectedIndex() const = 0;
      virtual bool getIsIntegrated() const = 0;
      virtual ~DimensionView() {};
    };
  }
}

#endif
