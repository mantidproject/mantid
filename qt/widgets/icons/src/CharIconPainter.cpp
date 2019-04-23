#include "../inc/MantidQtWidgets/CharIconPainter.h"

namespace MantidQt {
namespace Widgets {
namespace Icons {

void CharIconPainter::paint(IconicFont &iconic, QPainter *painter, QRect rect,
                            QIcon::Mode mode, QIcon::State state,
                            std::vector<QHash<QString, QVariant>> &options) {
  for (auto &option : options) {
    this->paintIcon(iconic, painter, rect, mode, state, option);
  }
}

void CharIconPainter::paintIcon(IconicFont &iconic, QPainter *painter,
                                QRect rect, QIcon::Mode mode,
                                QIcon::State state,
                                QHash<QString, QVariant> &options) {
  painter->save();
  const auto colorVariant = options[QString("color")];
  const auto scaleVariant = options[QString("scaleFactor")];
  const auto charecterStringVariant = options[QString("charecter")];
  const auto prefixVariant = options[QString("prefix")];
  const auto charecter = iconic.getCharmap()[prefixVariant.toString()]
                                            [charecterStringVariant.toString()];

  // Set some defaults so it doesn't fail later if nothing was set
  QString color("black");
  double scaleFactor(1.0);
  if (colorVariant.isString()) {
    color = colorVariant.toString();
  }
  if (scaleVariant.isString()) {
    scaleFactor = charVariant.toString();
  }
  painter.setPen(QColor(color));

  // A 16 pixel-high icon yields a font size of 14, which is pixel perfect for
  // font-awesome. 16 * 0.875 = 14 The reason why the glyph size is smaller than
  // the icon size is to account for font bearing.
  drawSize = 0.875 * unsigned long(rect.height() * scaleFactor);

  painter->setFont(iconic.font(m_prefix, draw_size));
  painter->setOpacity(1.0);
  painter->drawText(rect, Qt::AlignCenter | Qt::AlignVCenter, charecter);
  painter->restore();
}

} // namespace Icons
} // namespace Widgets
} // namespace MantidQt