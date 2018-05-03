#ifndef MANTIDQTMANTIDWIDGETS_CELL_H_
#define MANTIDQTMANTIDWIDGETS_CELL_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include <string>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON Cell {
public:
  Cell(std::string const &contentText);
  Cell(std::string const &contentText, std::string const& backgroundColor, int borderThickness,
       std::string const &color, int borderOpacity, bool isEditable);

  void setContentText(std::string const &contentText);
  std::string const &contentText() const;

  void setIconFilePath(std::string const& iconPath);
  std::string const& iconFilePath() const;

  void setBorderColor(std::string const &borderColor);
  std::string const &borderColor() const;

  void setBackgroundColor(std::string const &backgroundColor);
  std::string const &backgroundColor() const;

  void setBorderOpacity(int transparency);
  int borderOpacity() const;

  int borderThickness() const;
  void setBorderThickness(int borderThickness);

  bool isEditable() const;
  void setEditable(bool isEditable);
  void disableEditing();
  void enableEditing();

private:
  std::string m_contentText;
  std::string m_backgroundColor;
  int m_borderThickness;
  int m_borderOpacity;
  std::string m_borderColor;
  std::string m_iconFilePath;
  bool m_isEditable;
};

EXPORT_OPT_MANTIDQT_COMMON std::ostream &operator<<(std::ostream &os,
                                                    Cell const &cell);
EXPORT_OPT_MANTIDQT_COMMON bool operator==(Cell const &lhs, Cell const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator!=(Cell const &lhs, Cell const &rhs);
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_CELL_H_
