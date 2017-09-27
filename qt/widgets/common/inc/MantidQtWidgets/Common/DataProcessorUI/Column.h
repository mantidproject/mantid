#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOLUMN_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOLUMN_H
#include <QString>
#include "MantidQtWidgets/Common/DllOption.h"
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
class EXPORT_OPT_MANTIDQT_COMMON Column {
public:
  Column(QString const &name, QString const &algorithmProperty, bool isShown,
         QString const &prefix, QString const &description);
  QString const &name() const;
  QString const &algorithmProperty() const;
  bool isShown() const;
  QString const &prefix() const;
  QString const &description() const;

private:
  QString const &m_name;
  QString const &m_algorithmProperty;
  bool m_isShown;
  QString const &m_prefix;
  QString const &m_description;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_DATAPROCESSORCOLUMN_H
