#ifndef MANTIDQT_WIDGETS_ICONS_CHARICONPAINTER_H_
#define MANTIDQT_WIDGETS_ICONS_CHARICONPAINTER_H_

#include "DllOption.h"

#include <QIcon>
#include <QPainter>

namespace MantidQt {
namespace Widgets {
namespace Icons {

class IconicFont;

class EXPORT_OPT_MANTIDQT_ICONS CharIconPainter : public QPainter {
public:
  void paint(IconicFont *iconic, QPainter *painter, QRect rect,
             QIcon::Mode mode, QIcon::State state,
             std::vector<QHash<QString, QVariant>> &options);

private:
  void paintIcon(IconicFont *iconic, QPainter *painter, QRect rect,
                 QIcon::Mode mode, QIcon::State state,
                 QHash<QString, QVariant> &options);

  QString m_prefix;
};

} // namespace Icons
} // namespace Widgets
} // namespace MantidQt

#endif /* MANTIDQT_WIDGETS_ICONS_CHARICONPAINTER_H_ */