#include "MantidQtWidgets/Common/Batch/Row.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <boost/algorithm/string/predicate.hpp>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

Cell::Cell(std::string const &contentText)
    : m_contentText(contentText), m_borderThickness(1), m_borderColor("grey"),
      m_isEditable(false) {}

std::string const &Cell::contentText() const { return m_contentText; }

bool Cell::isEditable() const { return m_isEditable; }

int Cell::borderThickness() const { return m_borderThickness; }

std::string const &Cell::borderColor() const { return m_borderColor; }

void Cell::setContentText(std::string const &contentText) {
  m_contentText = contentText;
}

void Cell::setBorderThickness(int borderThickness) {
  m_borderThickness = borderThickness;
}

void Cell::setBorderColor(std::string const &borderColor) {
  m_borderColor = borderColor;
}

void Cell::setEditable(bool isEditable) {
  m_isEditable = isEditable;
}

void Cell::disableEditing() { m_isEditable = false; }

void Cell::enableEditing() { m_isEditable = true; }

std::ostream &operator<<(std::ostream &os, Cell const &cell) {
  os << '|' << cell.contentText() << '|';
  return os;
}

bool operator==(Cell const &lhs, Cell const &rhs) {
  return lhs.contentText() == rhs.contentText() &&
         lhs.isEditable() == rhs.isEditable() &&
         lhs.borderThickness() == rhs.borderThickness() &&
         lhs.borderColor() == rhs.borderColor();
}

Row::Row(RowLocation location, std::vector<Cell> cells)
    : m_location(std::move(location)), m_cells(std::move(cells)) {}

RowLocation const &Row::location() const { return m_location; }

std::vector<Cell> const &Row::cells() const { return m_cells; }
std::vector<Cell> &Row::cells() { return m_cells; }

std::ostream &operator<<(std::ostream &os, Row const &row) {
  os << row.location() << " ";
  for (auto &&cell : row.cells())
    os << cell;
  return os;
}

bool operator==(Row const &lhs, Row const &rhs) {
  return lhs.location() == rhs.location() && lhs.cells() == rhs.cells();
}

bool operator!=(Row const &lhs, Row const &rhs) { return !(lhs == rhs); }

bool operator<(Row const &lhs, Row const &rhs) {
  return lhs.location() < rhs.location();
}

bool operator<=(Row const &lhs, Row const &rhs) {
  return lhs < rhs || lhs == rhs;
}

bool operator>=(Row const &lhs, Row const &rhs) { return !(lhs < rhs); }

bool operator>(Row const &lhs, Row const &rhs) { return !(lhs <= rhs); }
}
}
}
