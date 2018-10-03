// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
    File                 : ScalePicker.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#include "ScalePicker.h"
#include "Graph.h"
#include "ScaleDraw.h"

#include <qwt_plot.h>
#include <qwt_scale_widget.h>
#include <qwt_text_label.h>

#include <QMouseEvent>
#include <QPen>

ScalePicker::ScalePicker(QwtPlot *plot)
    : QObject(plot), d_title_selected(false), d_labels_selected(false),
      d_selected_axis(nullptr), d_current_axis(nullptr) {
  refresh();
}

bool ScalePicker::eventFilter(QObject *object, QEvent *e) {
  if (!object->inherits("QwtScaleWidget"))
    return QObject::eventFilter(object, e);

  QwtScaleWidget *scale = static_cast<QwtScaleWidget *>(object);
  d_current_axis = scale;

  if (e->type() == QEvent::MouseButtonDblClick) {
    mouseDblClicked(scale, ((QMouseEvent *)e)->pos());
    return true;
  }

  if (e->type() == QEvent::MouseButtonPress) {
    const QMouseEvent *me = (const QMouseEvent *)e;
    QPoint pos = me->pos();
    if (me->button() == Qt::LeftButton) {
      scale->setFocus();
      emit clicked();

      deselect();

      if (titleRect(scale).contains(pos))
        selectTitle(scale);
      else if (!scaleTicksRect(scale).contains(pos))
        selectLabels(scale);

      return !(me->modifiers() & Qt::ShiftModifier) &&
             !scaleTicksRect(scale).contains(pos);
    } else if (me->button() == Qt::RightButton) {
      mouseRightClicked(scale, pos);
      return true;
    }
  }
  return QObject::eventFilter(object, e);
}

void ScalePicker::mouseDblClicked(const QwtScaleWidget *scale,
                                  const QPoint &pos) {
  if (titleRect(scale).contains(pos))
    emit axisTitleDblClicked();
  else if (scaleRect(scale).contains(pos))
    emit axisDblClicked(scale->alignment());
}

void ScalePicker::mouseRightClicked(const QwtScaleWidget *scale,
                                    const QPoint &pos) {
  if (scaleRect(scale).contains(pos))
    emit axisRightClicked(scale->alignment());
  else
    emit axisTitleRightClicked();
}

// The rect of a scale without the title
QRect ScalePicker::scaleRect(const QwtScaleWidget *scale) const {
  int margin = 1; // pixels tolerance
  QRect rect = scale->rect();
  rect.setRect(rect.x() - margin, rect.y() - margin, rect.width() + 2 * margin,
               rect.height() + 2 * margin);

  if (scale->title().text().isEmpty())
    return rect;

  int dh = scale->title().textSize().height();
  switch (scale->alignment()) {
  case QwtScaleDraw::LeftScale: {
    rect.setLeft(rect.left() + dh);
    break;
  }
  case QwtScaleDraw::RightScale: {
    rect.setRight(rect.right() - dh);
    break;
  }
  case QwtScaleDraw::BottomScale: {
    rect.setBottom(rect.bottom() - dh);
    break;
  }
  case QwtScaleDraw::TopScale: {
    rect.setTop(rect.top() + dh);
    break;
  }
  }
  return rect;
}

void ScalePicker::refresh() {
  for (uint i = 0; i < QwtPlot::axisCnt; i++) {
    QwtScaleWidget *scale =
        static_cast<QwtScaleWidget *>(plot()->axisWidget(i));
    if (scale)
      scale->installEventFilter(this);
  }
}

QwtPlot *ScalePicker::plot() { return dynamic_cast<QwtPlot *>(parent()); }

QRect ScalePicker::scaleTicksRect(const QwtScaleWidget *scale) const {
  int majTickLength = scale->scaleDraw()->majTickLength();
  QRect rect = scale->rect();
  switch (scale->alignment()) {
  case QwtScaleDraw::LeftScale:
    rect.setLeft(rect.right() - majTickLength);
    break;
  case QwtScaleDraw::RightScale:
    rect.setRight(rect.left() + majTickLength);
    break;
  case QwtScaleDraw::TopScale:
    rect.setTop(rect.bottom() - majTickLength);
    break;
  case QwtScaleDraw::BottomScale:
    rect.setBottom(rect.top() + majTickLength);
    break;
  }
  return rect;
}

QRect ScalePicker::titleRect(const QwtScaleWidget *scale) const {
  if (scale->title().text().isEmpty())
    return QRect();

  QRect rect = scale->rect();
  int margin = scale->margin();
  rect = rect.adjusted(margin, margin, -margin, -margin);

  int dh = scale->title().textSize().height();
  switch (scale->alignment()) {
  case QwtScaleDraw::LeftScale: {
    rect.setRight(rect.left() + dh);
    break;
  }
  case QwtScaleDraw::RightScale: {
    rect.setLeft(rect.right() - dh);
    break;
  }
  case QwtScaleDraw::BottomScale: {
    rect.setTop(rect.bottom() - dh);
    break;
  }
  case QwtScaleDraw::TopScale: {
    rect.setBottom(rect.top() + dh);
    break;
  }
  }
  return rect;
}

void ScalePicker::selectTitle(QwtScaleWidget *scale, bool select) {
  if (!scale)
    return;

  if (d_title_selected == select && d_selected_axis == scale)
    return;

  Graph *g = static_cast<Graph *>(plot()->parent());
  g->deselect();

  d_title_selected = select;
  d_selected_axis = scale;
  d_labels_selected = false;

  QwtText title = scale->title();
  if (select) {
    title.setBackgroundPen(QPen(Qt::blue));
    g->notifyFontChange(title.font());
  } else
    title.setBackgroundPen(QPen(Qt::NoPen));

  scale->setTitle(title);
}

void ScalePicker::selectLabels(QwtScaleWidget *scale, bool select) {
  if (!scale)
    return;

  if (d_labels_selected == select && d_selected_axis == scale)
    return;

  Graph *g = static_cast<Graph *>(plot()->parent());
  g->deselect();

  d_labels_selected = select;
  d_selected_axis = scale;
  d_title_selected = false;

  g->notifyFontChange(scale->font());

  ScaleDraw *sc_draw = static_cast<ScaleDraw *>(scale->scaleDraw());
  sc_draw->setSelected(select);
  scale->repaint();
}

void ScalePicker::deselect() {
  if (!d_selected_axis)
    return;

  d_title_selected = false;
  d_labels_selected = false;

  QwtText title = d_selected_axis->title();
  title.setBackgroundPen(QPen(Qt::NoPen));
  d_selected_axis->setTitle(title);

  ScaleDraw *sc_draw = static_cast<ScaleDraw *>(d_selected_axis->scaleDraw());
  sc_draw->setSelected(false);

  d_selected_axis->repaint();
  d_selected_axis = nullptr;
}
