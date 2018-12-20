#ifndef LOADVTK_H_
#define LOADVTK_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/System.h"

class vtkUnsignedShortArray;
class vtkDataSet;

namespace Mantid {
namespace API {
class Progress;
}
namespace VATES {
class DLLExport LoadVTK : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  const std::string name() const override;

  int version() const override;

  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a legacy binary format VTK uniform structured image as "
           "an MDWorkspace.";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  void execMDHisto(vtkUnsignedShortArray *signals,
                   vtkUnsignedShortArray *errorsSQ,
                   Mantid::Geometry::MDHistoDimension_sptr dimX,
                   Mantid::Geometry::MDHistoDimension_sptr dimY,
                   Mantid::Geometry::MDHistoDimension_sptr dimZ,
                   Mantid::API::Progress &prog, const int64_t nPoints,
                   const int64_t frequency);

  void execMDEvent(vtkDataSet *readDataset, vtkUnsignedShortArray *signals,
                   vtkUnsignedShortArray *errorsSQ,
                   Mantid::Geometry::MDHistoDimension_sptr dimX,
                   Mantid::Geometry::MDHistoDimension_sptr dimY,
                   Mantid::Geometry::MDHistoDimension_sptr dimZ,
                   Mantid::API::Progress &prog, const int64_t nPoints,
                   const int64_t frequency);

  void init() override;

  void exec() override;
};
} // namespace VATES
} // namespace Mantid

#endif
