#include "MantidQtMantidWidgets/CheckboxHeader.h"

namespace MantidQt
{
  namespace MantidWidgets
  {

    /**
     * Constructor
     * @param orientation :: The orientation (horizontal or vertical) of the header.
     * @param parent      :: The table to apply the header to.
     */
    CheckboxHeader::CheckboxHeader(Qt::Orientation orientation, QWidget *parent) : QHeaderView(orientation, parent), m_checked(false)
    {
      show();
      setClickable(true);
    }


    /**
     * Has the user checked the checkbox in the header?
     * @param checked :: True if user has checked the checkbox in the header.
     */
    void CheckboxHeader::setChecked(bool checked)
    {
      if (isEnabled() && m_checked != checked)
      {
        m_checked = checked;
        updateSection(0);
        emit toggled(m_checked);
      }
    }

    /**
     * Implements the checkbox functionality into the first column of the table.
     * @param painter :: Paints the specific widget.
     * @param rect    :: The area to paint.
     * @param logicalIndex :: The column in the table.
     */
    void CheckboxHeader::paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const
    {
      painter->save();
      QHeaderView::paintSection(painter, rect, logicalIndex);
      painter->restore();

      if (logicalIndex == 0)
      {
        // We have to clear the data otherwise the default "1" appears.
        model()->setHeaderData(0, Qt::Horizontal, tr(""));

        QStyleOptionButton option;

        if (isEnabled())
        {
          option.state |= QStyle::State_Enabled;
        }

        option.rect = checkBoxRect(rect);
        option.state |= m_checked ? QStyle::State_On : QStyle::State_Off;

        style()->drawControl(QStyle::CE_CheckBox, &option, painter);
      }

    }

    /**
     * Set the checkbox to checked if clicked, otherwise unchecked.
     * @param event :: The mouse event the user performs.
     */
    void CheckboxHeader::mousePressEvent(QMouseEvent *event)
    {
      if (isEnabled() && logicalIndexAt(event->pos()) == 0)
      {
        m_checked = !m_checked;
        updateSection(0);
        emit toggled(m_checked);
      }
      else
      {
        QHeaderView::mousePressEvent(event);
      }
    }

    QRect CheckboxHeader::checkBoxRect(const QRect &sourceRect) const
    {
      QStyleOptionButton checkBoxStyleOption;

      QRect checkBoxRect = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &checkBoxStyleOption);

      QPoint checkBoxPoint(sourceRect.x() + 3,sourceRect.y() + sourceRect.height() / 2 - checkBoxRect.height() / 2);

      return QRect(checkBoxPoint, checkBoxRect.size());
    }

  } // namespace MantidWidgets
} // namespace MantidQt
