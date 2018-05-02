#include "MantidQtWidgets/Common/Batch/Cell.h"
#include <ostream>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

Cell::Cell(std::string const &contentText, int borderThickness,
           std::string const &borderColor, int borderTransparency, bool isEditable)
    : m_contentText(contentText), m_borderThickness(borderThickness),
      m_borderTransparency(borderTransparency), m_borderColor(borderColor),
      m_isEditable(isEditable) {}

Cell::Cell(std::string const &contentText)
    : m_contentText(contentText), m_borderThickness(1), m_borderTransparency(255),
      m_borderColor("darkGrey"), m_isEditable(true) {}

std::string const &Cell::contentText() const { return m_contentText; }

bool Cell::isEditable() const { return m_isEditable; }

int Cell::borderThickness() const { return m_borderThickness; }

std::string const &Cell::borderColor() const { return m_borderColor; }

int Cell::borderTransparency() const { return m_borderTransparency; }

void Cell::setContentText(std::string const &contentText) {
  m_contentText = contentText;
}

void Cell::setBorderThickness(int borderThickness) {
  m_borderThickness = borderThickness;
}

void Cell::setBorderColor(std::string const &borderColor) {
  m_borderColor = borderColor;
}

void Cell::setBorderTransparency(int borderTransparency) {
  m_borderTransparency = borderTransparency;
}

void Cell::setEditable(bool isEditable) { m_isEditable = isEditable; }

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

bool operator!=(Cell const &lhs, Cell const &rhs) { return !(lhs == rhs); }
}
}
}
