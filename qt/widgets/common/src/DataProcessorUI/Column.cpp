#include "MantidQtWidgets/Common/DataProcessorUI/Column.h"
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

Column::Column(QString const &name, QString const &algorithmProperty,
               bool isShown, QString const &prefix, QString const &description)
    : m_name(name), m_algorithmProperty(algorithmProperty), m_isShown(isShown),
      m_prefix(prefix), m_description(description) {}

QString const &Column::algorithmProperty() const { return m_algorithmProperty; }

bool Column::isShown() const { return m_isShown; }

QString const &Column::prefix() const { return m_prefix; }

QString const &Column::description() const { return m_description; }

QString const &Column::name() const { return m_name; }
}
}
}
