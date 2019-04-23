#ifndef MANTIDQT_WIDGETS_ICONS_CHARICONPAINTER_H_
#define MANTIDQT_WIDGETS_ICONS_CHARICONPAINTER_H_

namespace MantidQt {
namespace Widgets {
namespace Icons {

class CharIconPainter : QPainter {
public:
  void paint(IconicFont &iconic, QPainter *painter, QRect rect,
             QIcon::Mode mode, QIcon::State state,
             std::vector<QHash<QString, QVariant>> &options);

private:
  void paintIcon(IconicFont &iconic, QPainter *painter, QRect rect,
                 QIcon::Mode mode, QIcon::State state,
                 QHash<QString, QVariant> &options);
};

} // namespace Icons
} // namespace Widgets
} // namespace MantidQt

#endif /* MANTIDQT_WIDGETS_ICONS_CHARICONPAINTER_H_ */