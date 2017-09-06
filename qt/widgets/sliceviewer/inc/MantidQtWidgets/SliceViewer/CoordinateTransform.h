#ifndef MANTIDQT_SLICEVIEWER_COORDINATETRANSFORM_H
#define MANTIDQT_SLICEVIEWER_COORDINATETRANSFORM_H

#include "MantidKernel/VMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include "DllOption.h"
#include <array>
#include <memory>

namespace MantidQt {
namespace SliceViewer {
class EXPORT_OPT_MANTIDQT_SLICEVIEWER CoordinateTransform {
public:
  virtual ~CoordinateTransform() {}
  virtual void transform(Mantid::Kernel::VMD &coords, size_t dimX, size_t dimY,
                         size_t missingHKLDim) = 0;
  virtual void checkDimensionsForHKL(const Mantid::API::IMDWorkspace &ws,
                                     size_t dimX, size_t dimY) = 0;
};

class EXPORT_OPT_MANTIDQT_SLICEVIEWER NullTransform
    : public CoordinateTransform {
public:
  void transform(Mantid::Kernel::VMD &coords, size_t dimX, size_t dimY,
                 size_t missingHKLDim) override;
  void checkDimensionsForHKL(const Mantid::API::IMDWorkspace &ws, size_t dimX,
                             size_t dimY) override;
};

class EXPORT_OPT_MANTIDQT_SLICEVIEWER NonOrthogonalTransform
    : public CoordinateTransform {
public:
  ~NonOrthogonalTransform();
  NonOrthogonalTransform(const Mantid::API::IMDWorkspace &workspace,
                         size_t dimX, size_t dimY);
  void transform(Mantid::Kernel::VMD &coords, size_t dimX, size_t dimY,
                 size_t missingHKLDim) override;
  void checkDimensionsForHKL(const Mantid::API::IMDWorkspace &ws, size_t dimX,
                             size_t dimY) override;

private:
  bool m_dimensionsHKL;
  std::array<Mantid::coord_t, 9> m_skewMatrix;
};

std::unique_ptr<CoordinateTransform> EXPORT_OPT_MANTIDQT_SLICEVIEWER
createCoordinateTransform(const Mantid::API::IMDWorkspace &ws, size_t dimX,
                          size_t dimY);
}
}
#endif
