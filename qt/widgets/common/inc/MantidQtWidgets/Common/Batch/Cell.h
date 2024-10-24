// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#pragma once
#include "MantidQtWidgets/Common/DllOption.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON Cell {
public:
  // Cell values may be user inputs or algorithm outputs
  enum class Direction { INPUT, OUTPUT };
  static constexpr const char *INPUT_FOREGROUND_COLOR = "#000000";
  static constexpr const char *OUTPUT_FOREGROUND_COLOR = "#808080";

  Cell(std::string contentText);
  Cell(std::string contentText, std::string backgroundColor, int borderThickness, std::string color, int borderOpacity,
       bool isEditable);

  void setContentText(std::string const &contentText);
  std::string const &contentText() const;

  void setIconFilePath(std::string const &iconPath);
  std::string const &iconFilePath() const;

  void setBorderColor(std::string const &borderColor);
  std::string const &borderColor() const;

  void setBackgroundColor(std::string const &backgroundColor);
  std::string const &backgroundColor() const;
  void setForegroundColor(std::string const &foregroundColor);
  std::string const &foregroundColor() const;

  void setBorderOpacity(int transparency);
  int borderOpacity() const;

  int borderThickness() const;
  void setBorderThickness(int borderThickness);

  void setToolTip(std::string const &toolTip);
  std::string const &toolTip() const;

  bool isEditable() const;
  void setEditable(bool isEditable);
  void disableEditing();
  void enableEditing();

  bool isInput() const;
  bool isOutput() const;
  void setInput();
  void setOutput();

private:
  std::string m_contentText;
  std::string m_backgroundColor;
  std::string m_foregroundColor;
  int m_borderThickness;
  int m_borderOpacity;
  std::string m_borderColor;
  std::string m_iconFilePath;
  bool m_isEditable;
  std::string m_toolTip;
  Direction m_direction;
};

EXPORT_OPT_MANTIDQT_COMMON std::ostream &operator<<(std::ostream &os, Cell const &cell);
EXPORT_OPT_MANTIDQT_COMMON bool operator==(Cell const &lhs, Cell const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator!=(Cell const &lhs, Cell const &rhs);
EXPORT_OPT_MANTIDQT_COMMON std::vector<Cell> paddedCellsToWidth(std::vector<Cell> const &cells, Cell const &paddingCell,
                                                                int paddedWidth);
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
