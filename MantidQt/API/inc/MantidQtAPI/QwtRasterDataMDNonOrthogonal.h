#ifndef QwtRasterDataMDNonOrthogonal_H_
#define QwtRasterDataMDNonOrthogonal_H_

#include "MantidQtAPI/DllOption.h"
#include "MantidQtAPI/QwtRasterDataMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Matrix.h"
#include <array>

namespace MantidQt {
namespace API {

class EXPORT_OPT_MANTIDQT_API QwtRasterDataMDNonOrthogonal
    : public QwtRasterDataMD {
public:
  QwtRasterDataMDNonOrthogonal();
  QwtRasterDataMDNonOrthogonal *copy() const override;

  void setWorkspace(Mantid::API::IMDWorkspace_const_sptr ws) override;

  double value(double x, double y) const override;

  void setSliceParams(size_t dimX, size_t dimY,
                      Mantid::Geometry::IMDDimension_const_sptr X,
                      Mantid::Geometry::IMDDimension_const_sptr Y,
                      std::vector<Mantid::coord_t> &slicePoint) override;
  mutable std::vector<Mantid::coord_t> m_lookPoint;
  std::array<Mantid::coord_t, 9> m_fromHklToXyz;
  size_t m_missingHKLdim;

protected:
  void copyFrom(const QwtRasterDataMDNonOrthogonal &source,
                QwtRasterDataMDNonOrthogonal &dest) const;
};

} // namespace SliceViewer
} // namespace Mantid

#endif /* QwtRasterDataMDNonOrthogonal_H_ */
