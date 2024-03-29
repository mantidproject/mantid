/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/

#include "MantidQtWidgets/Common/LineEditWithClear.h"
#include <QStyle>
#include <QToolButton>

namespace MantidQt::MantidWidgets {

LineEditWithClear::LineEditWithClear(QWidget *parent) : QLineEdit(parent) {
  clearButton = new QToolButton(this);
  QPixmap pixmap(QString::fromUtf8(":/fileclose.png"));
  clearButton->setIcon(QIcon(pixmap));
  clearButton->setIconSize(pixmap.size());
  clearButton->setCursor(Qt::ArrowCursor);
  clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  clearButton->hide();
  connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
  connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(updateCloseButton(const QString &)));
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(clearButton->sizeHint().width() + frameWidth + 1));
  QSize msz = minimumSizeHint();
  setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
                 qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void LineEditWithClear::resizeEvent(QResizeEvent * /*unused*/) {
  QSize sz = clearButton->sizeHint();
  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  clearButton->move(rect().right() - frameWidth - sz.width(), (rect().bottom() + 1 - sz.height()) / 2);
}

void LineEditWithClear::updateCloseButton(const QString &text) { clearButton->setVisible(!text.isEmpty()); }
} // namespace MantidQt::MantidWidgets
