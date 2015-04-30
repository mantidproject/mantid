#ifndef LOADVTK_H_
#define LOADVTK_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidAPI/IFileLoader.h"

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
    class DLLExport LoadVTK : public API::IFileLoader<Kernel::FileDescriptor>
    {
    public:
      virtual const std::string name() const;

      virtual int version() const;

      virtual const std::string category() const;

      /// Summary of algorithms purpose
      virtual const std::string summary() const {return "Loads a legacy binary format VTK uniform structured image as an MDWorkspace.";}

      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::FileDescriptor & descriptor) const;

    private:

      void execMDHisto(vtkUnsignedShortArray* signals, vtkUnsignedShortArray* errorsSQ, Mantid::Geometry::MDHistoDimension_sptr dimX, Mantid::Geometry::MDHistoDimension_sptr dimY, Mantid::Geometry::MDHistoDimension_sptr dimZ, Mantid::API::Progress& prog, const int64_t nPoints, const int64_t frequency);

      void execMDEvent(vtkDataSet* readDataset,vtkUnsignedShortArray* signals, vtkUnsignedShortArray* errorsSQ, Mantid::Geometry::MDHistoDimension_sptr dimX, Mantid::Geometry::MDHistoDimension_sptr dimY, Mantid::Geometry::MDHistoDimension_sptr dimZ, Mantid::API::Progress& prog, const int64_t nPoints, const int64_t frequency);

      virtual void init();

      virtual void exec();
    };
  }
}

#endif
