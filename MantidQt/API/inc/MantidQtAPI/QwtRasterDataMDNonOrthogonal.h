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

protected:
  void copyFrom(const QwtRasterDataMDNonOrthogonal &source,
                QwtRasterDataMDNonOrthogonal &dest) const;
  Mantid::coord_t *m_lookPoint;
  Mantid::coord_t m_skewMatrix[9];
};

} // namespace SliceViewer
} // namespace Mantid

#endif /* QwtRasterDataMDNonOrthogonal_H_ */