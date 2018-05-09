#ifndef MANTIDQTMANTIDWIDGETS_ROWLOCATIONADAPTER_H_
#define MANTIDQTMANTIDWIDGETS_ROWLOCATIONADAPTER_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include <QStandardItemModel>
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/StrictQModelIndices.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON RowLocationAdapter {
public:
  RowLocationAdapter(QStandardItemModel const& model);

  RowLocation atIndex(QModelIndexForMainModel const &index) const;
  boost::optional<QModelIndexForMainModel>
  indexIfExistsAt(RowLocation const &location, int column = 0) const;
  QModelIndexForMainModel indexAt(RowLocation const &location,
                                  int column = 0) const;

private:
  QStandardItemModel const &m_model;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_ROWLOCATIONADAPTER_H_
