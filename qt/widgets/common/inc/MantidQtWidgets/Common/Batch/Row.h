#ifndef MANTIDQTMANTIDWIDGETS_ROW_H_
#define MANTIDQTMANTIDWIDGETS_ROW_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include <boost/optional.hpp>
#include <ostream>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class Cell {
public:
  Cell(std::string const &contentText);

  std::string const &contentText() const;
  int borderThickness() const;
  std::string const& borderColor() const;
  bool isEditable() const;

  void setContentText(std::string const& contentText);
  void setBorderThickness(int borderThickness);
  void setBorderColor(std::string const& borderColor);
  void setEditable(bool isEditable);
  void disableEditing();
  void enableEditing();

private:
  std::string m_contentText;
  int m_borderThickness;
  std::string m_borderColor;
  bool m_isEditable;
};

class EXPORT_OPT_MANTIDQT_COMMON Row {
public:
  Row(RowLocation location, std::vector<Cell> cells);

  std::vector<Cell> const &cells() const;
  std::vector<Cell> &cells();

  RowLocation const &location() const;

private:
  RowLocation m_location;
  std::vector<Cell> m_cells;
};

using Subtree = std::vector<Row>;

EXPORT_OPT_MANTIDQT_COMMON std::ostream &operator<<(std::ostream &os,
                                                    Row const &location);
EXPORT_OPT_MANTIDQT_COMMON bool operator==(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator!=(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator<(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator<=(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator>(Row const &lhs, Row const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator>=(Row const &lhs, Row const &rhs);
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif
