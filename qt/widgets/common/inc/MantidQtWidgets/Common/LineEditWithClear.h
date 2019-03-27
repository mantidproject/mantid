/****************************************************************************
**
** Copyright (c) 2007 Trolltech ASA <info@trolltech.com>
**
** Use, modification and distribution is allowed without limitation,
** warranty, liability or support of any kind.
**
****************************************************************************/

#ifndef LINEEDITWITHCLEAR_H
#define LINEEDITWITHCLEAR_H

#include "DllOption.h"
#include "MantidKernel/System.h"
#include <QLineEdit>

class QToolButton;

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON LineEditWithClear : public QLineEdit {
  Q_OBJECT

public:
  LineEditWithClear(QWidget *parent = nullptr);

protected:
  void resizeEvent(QResizeEvent * /*unused*/) override;

private slots:
  void updateCloseButton(const QString &text);

private:
  QToolButton *clearButton;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // LINEEDITWITHCLEAR_H
