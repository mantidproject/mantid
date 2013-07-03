#ifndef LOADVTK_H_
#define LOADVTK_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidMDEvents/MDEventWorkspace.h"

class vtkUnsignedShortArray;
class vtkDataSet;

namespace Mantid
{
  namespace API
  {
    class Progress;
  }
  namespace VATES
  {
    class DLLExport LoadVTK : public Mantid::API::Algorithm
    {
    public:
      virtual const std::string name() const;

      virtual int version() const;

      virtual const std::string category() const;

    protected:

      virtual void initDocs();

    private:

      void execMDHisto(vtkUnsignedShortArray* signals, vtkUnsignedShortArray* errorsSQ, Mantid::Geometry::MDHistoDimension_sptr dimX, Mantid::Geometry::MDHistoDimension_sptr dimY, Mantid::Geometry::MDHistoDimension_sptr dimZ, Mantid::API::Progress& prog, const int64_t nPoints, const int64_t frequency);

      void execMDEvent(vtkDataSet* readDataset,vtkUnsignedShortArray* signals, vtkUnsignedShortArray* errorsSQ, Mantid::Geometry::MDHistoDimension_sptr dimX, Mantid::Geometry::MDHistoDimension_sptr dimY, Mantid::Geometry::MDHistoDimension_sptr dimZ, Mantid::API::Progress& prog, const int64_t nPoints, const int64_t frequency);

      virtual void init();

      virtual void exec();
    };
  }
}

#endif
