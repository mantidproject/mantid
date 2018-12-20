// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : TitlePicker.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "TitlePicker.h"

#include <qwt_plot.h>
#include <qwt_text_label.h>

#include <QMouseEvent>
#include <QPen>

TitlePicker::TitlePicker(QwtPlot *plot) : QObject(plot) {
  d_selected = false;
  title = plot->titleLabel();
  title->setFocusPolicy(Qt::StrongFocus);
  if (title)
    title->installEventFilter(this);
}

bool TitlePicker::eventFilter(QObject *object, QEvent *e) {
  if (object != (QObject *)title)
    return FALSE;

  if (object->inherits("QwtTextLabel") &&
      e->type() == QEvent::MouseButtonDblClick) {
    emit doubleClicked();
    d_selected = true;
    return TRUE;
  }

  if (object->inherits("QwtTextLabel") &&
      e->type() == QEvent::MouseButtonPress) {
    const QMouseEvent *me = (const QMouseEvent *)e;

    emit clicked();

    if (me->button() == Qt::RightButton)
      emit showTitleMenu();
    else if (me->button() == Qt::LeftButton)
      setSelected();

    return !(me->modifiers() & Qt::ShiftModifier);
  }

  if (object->inherits("QwtTextLabel") && e->type() == QEvent::KeyPress) {
    switch (((const QKeyEvent *)e)->key()) {
    case Qt::Key_Delete:
      emit removeTitle();
      return TRUE;
    }
  }

  return QObject::eventFilter(object, e);
}

void TitlePicker::setSelected(bool select) {
  if (!title || d_selected == select)
    return;

  d_selected = select;

  QwtText text = title->text();
  if (select)
    text.setBackgroundPen(QPen(Qt::blue));
  else
    text.setBackgroundPen(QPen(Qt::NoPen));

  (static_cast<QwtPlot *>(parent()))->setTitle(text);
}
