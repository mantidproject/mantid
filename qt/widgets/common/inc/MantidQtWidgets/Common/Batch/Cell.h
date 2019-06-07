// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#ifndef MANTIDQTMANTIDWIDGETS_CELL_H_
#define MANTIDQTMANTIDWIDGETS_CELL_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON Cell {
public:
  Cell(std::string const &contentText);
  Cell(std::string const &contentText, std::string const &backgroundColor,
       int borderThickness, std::string const &color, int borderOpacity,
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

  bool containsOutputValue() const;
  void setContainsOutputValue(bool containsOutputValue);

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
  bool m_containsOutputValue;
};

EXPORT_OPT_MANTIDQT_COMMON std::ostream &operator<<(std::ostream &os,
                                                    Cell const &cell);
EXPORT_OPT_MANTIDQT_COMMON bool operator==(Cell const &lhs, Cell const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator!=(Cell const &lhs, Cell const &rhs);
EXPORT_OPT_MANTIDQT_COMMON std::vector<Cell>
paddedCellsToWidth(std::vector<Cell> const &cells, Cell const &paddingCell,
                   int paddedWidth);
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_CELL_H_
