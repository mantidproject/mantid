// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtIcons/CharIconPainter.h"
#include "MantidQtIcons/Icon.h"
#include <QVariant>
#include <cmath>
#include <stdexcept>

namespace MantidQt::Icons {

namespace {
const QString DEFAULT_COLOR("black");
const QString DEFAULT_DEACTIVATED_COLOR("#c7c7c7");
} // namespace

void CharIconPainter::paint(IconicFont *iconic, QPainter *painter, QRect rect, QIcon::Mode mode, QIcon::State state,
                            QList<QHash<QString, QVariant>> &options) {
  for (auto &option : options) {
    this->paintIcon(iconic, painter, rect, mode, state, option);
  }
}

void CharIconPainter::paintIcon(IconicFont *iconic, QPainter *painter, QRect rect, QIcon::Mode mode, QIcon::State state,
                                QHash<QString, QVariant> &options) {
  // These variables will be useful later on, so I have left them being passed
  // down this far. This is because they can be used for defining variable
  // changes based on state of the buttons/QObject that the Icon is present in.
  // Since we currently don't use this feature available with QIcon they have
  // not yet been implemented. Mode has now been implemented however it will
  // only ever be gray in its current mode when disabled.
  UNUSED_ARG(state);

  painter->save();
  const auto colorVariant = options[QString("color")];
  const auto scaleVariant = options[QString("scaleFactor")];
  const auto charecterOption = options[QString("character")].toString();
  const auto prefix = options[QString("prefix")].toString();
  const auto charecter = iconic->findCharacterFromCharMap(prefix, charecterOption);

  // Set some defaults so it doesn't fail later if nothing was set
  QString color(DEFAULT_COLOR);
  double scaleFactor(1.0);
  if (mode == QIcon::Mode::Disabled) {
    color = DEFAULT_DEACTIVATED_COLOR; // gray
  } else if (colorVariant.canConvert<QString>()) {
    color = colorVariant.toString();
  }
  if (scaleVariant.canConvert<double>()) {
    scaleFactor = scaleVariant.toDouble();
  }
  painter->setPen(QColor(color));

  // A 16 pixel-high icon yields a font size of 14, which is pixel perfect for
  // font-awesome. 16 * 0.875 = 14 The reason why the glyph size is smaller than
  // the icon size is to account for font bearing.
  const auto drawSize = static_cast<int>(std::floor(0.875 * std::round(rect.height() * scaleFactor)));

  bool unicodeGenerationSuccess;
  const auto unicode = QChar(charecter.toUInt(&unicodeGenerationSuccess, 16));
  if (!unicodeGenerationSuccess) {
    throw std::invalid_argument("Failed to convert hex value: \"" + charecter.toStdString() + "\" to unicode.");
  }

  painter->setFont(iconic->getFont(prefix, drawSize));
  painter->setOpacity(1.0);
  painter->drawText(rect, Qt::AlignCenter | Qt::AlignVCenter, unicode);
  painter->restore();
}

} // namespace MantidQt::Icons
