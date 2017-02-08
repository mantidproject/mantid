#ifndef QwtRasterDataMDNonOrthogonal_H_
#define QwtRasterDataMDNonOrthogonal_H_

#include "MantidQtAPI/DllOption.h"
#include "MantidQtAPI/QwtRasterDataMD.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Matrix.h"

namespace MantidQt {
namespace API {

class EXPORT_OPT_MANTIDQT_API QwtRasterDataMDNonOrthogonal
    : public QwtRasterDataMD {
public:
  QwtRasterDataMDNonOrthogonal();
  ~QwtRasterDataMDNonOrthogonal() override;
  QwtRasterDataMDNonOrthogonal *copy() const override;

  void setWorkspace(Mantid::API::IMDWorkspace_const_sptr ws) override;

  double value(double x, double y) const override;

  void setSliceParams(size_t dimX, size_t dimY,
                      Mantid::Geometry::IMDDimension_const_sptr X,
                      Mantid::Geometry::IMDDimension_const_sptr Y,
                      std::vector<Mantid::coord_t> &slicePoint) override;
  Mantid::coord_t *m_lookPoint;
  Mantid::coord_t m_fromHklToXyz[9];
  size_t m_missingHKLdim;

protected:
  void copyFrom(const QwtRasterDataMDNonOrthogonal &source,
                QwtRasterDataMDNonOrthogonal &dest) const;
};

} // namespace SliceViewer
} // namespace Mantid

#endif /* QwtRasterDataMDNonOrthogonal_H_ */
