// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Batch/Cell.h"
#include <ostream>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

Cell::Cell(std::string const &contentText, std::string const &backgroundColor,
           int borderThickness, std::string const &borderColor,
           int borderOpacity, bool isEditable)
    : m_contentText(contentText), m_backgroundColor(backgroundColor),
      m_borderThickness(borderThickness), m_borderOpacity(borderOpacity),
      m_borderColor(borderColor), m_iconFilePath(), m_isEditable(isEditable),
      m_toolTip(""), m_direction(Direction::INPUT) {}

Cell::Cell(std::string const &contentText)
    : m_contentText(contentText), m_backgroundColor("white"),
      m_borderThickness(1), m_borderOpacity(255), m_borderColor("darkGrey"),
      m_iconFilePath(), m_isEditable(true), m_toolTip(""),
      m_direction(Direction::INPUT) {}

std::string const &Cell::contentText() const { return m_contentText; }

std::string const &Cell::toolTip() const { return m_toolTip; }

bool Cell::isEditable() const { return m_isEditable; }

int Cell::borderThickness() const { return m_borderThickness; }

std::string const &Cell::borderColor() const { return m_borderColor; }

int Cell::borderOpacity() const { return m_borderOpacity; }

void Cell::setContentText(std::string const &contentText) {
  m_contentText = contentText;
}

void Cell::setToolTip(std::string const &toolTip) { m_toolTip = toolTip; }

void Cell::setBorderThickness(int borderThickness) {
  m_borderThickness = borderThickness;
}

void Cell::setBorderColor(std::string const &borderColor) {
  m_borderColor = borderColor;
}

void Cell::setBackgroundColor(std::string const &backgroundColor) {
  m_backgroundColor = backgroundColor;
}

void Cell::setForegroundColor(std::string const &foregroundColor) {
  m_foregroundColor = foregroundColor;
  if (m_foregroundColor == OUTPUT_FOREGROUND_COLOR)
    m_direction = Direction::OUTPUT;
  else
    m_direction = Direction::INPUT;
}

std::string const &Cell::backgroundColor() const { return m_backgroundColor; }

std::string const &Cell::foregroundColor() const { return m_foregroundColor; }

void Cell::setBorderOpacity(int borderOpacity) {
  m_borderOpacity = borderOpacity;
}

void Cell::setIconFilePath(std::string const &iconFilePath) {
  m_iconFilePath = iconFilePath;
}

std::string const &Cell::iconFilePath() const { return m_iconFilePath; }

void Cell::setEditable(bool isEditable) { m_isEditable = isEditable; }

void Cell::disableEditing() { m_isEditable = false; }

void Cell::enableEditing() { m_isEditable = true; }

bool Cell::isInput() const { return m_direction == Direction::INPUT; }

bool Cell::isOutput() const { return m_direction == Direction::OUTPUT; }

void Cell::setInput() {
  m_direction = Direction::INPUT;
  m_foregroundColor = INPUT_FOREGROUND_COLOR;
}

void Cell::setOutput() {
  m_direction = Direction::OUTPUT;
  m_foregroundColor = OUTPUT_FOREGROUND_COLOR;
}

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

std::vector<Cell> paddedCellsToWidth(std::vector<Cell> const &cells,
                                     Cell const &paddingCell, int paddedWidth) {
  auto paddedCells = cells;
  for (auto i = static_cast<int>(cells.size()); i < paddedWidth; i++)
    paddedCells.emplace_back(paddingCell);
  return paddedCells;
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
