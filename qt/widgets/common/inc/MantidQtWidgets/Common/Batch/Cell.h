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
  Cell(std::string const &contentText, int borderThickness,
       std::string const &color, int borderTransparency, bool isEditable);

  std::string const &contentText() const;
  int borderThickness() const;
  std::string const &borderColor() const;
  int borderTransparency() const;
  bool isEditable() const;

  void setContentText(std::string const &contentText);
  void setBorderThickness(int borderThickness);
  void setBorderColor(std::string const &borderColor);
  void setBorderTransparency(int transparency);

  void setEditable(bool isEditable);
  void disableEditing();
  void enableEditing();

private:
  std::string m_contentText;
  int m_borderThickness;

  int m_borderTransparency;
  std::string m_borderColor;

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
