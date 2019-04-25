// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_ICONS_CHARICONPAINTER_H_
#define MANTIDQT_ICONS_CHARICONPAINTER_H_

#include "DllOption.h"

#include <QIcon>
#include <QPainter>

namespace MantidQt {
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
};

} // namespace Icons
} // namespace MantidQt

#endif /* MANTIDQT_WIDGETS_ICONS_CHARICONPAINTER_H_ */