#include "MantidQtAPI/PropertyInfoWidget.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QPair>

namespace MantidQt
{
namespace API
{
  /**
   * Constructor.
   *
   * @param parent :: this widget's parent
   */
  PropertyInfoWidget::PropertyInfoWidget(QWidget *parent)
    : QFrame(parent), m_labels()
  {
    setLayout(new QHBoxLayout(this));
    layout()->setSpacing(1);
    layout()->setContentsMargins(0, 0, 0, 0);
    
    QMap<Info, QPair<QString, QString>> pathsAndToolTips;    
    pathsAndToolTips[REPLACE] = QPair<QString, QString>(":/replace.png", "A workspace with this name already exists and so will be overwritten.");
    pathsAndToolTips[RESTORE] = QPair<QString, QString>(":/restore.png", "This was a previously-entered value.");

    foreach( const auto info, pathsAndToolTips.keys() )
    {
      const QString iconPath = pathsAndToolTips[info].first;
      const QString toolTip  = pathsAndToolTips[info].second;
      
      auto label = new QLabel(this);
      label->setPixmap(QPixmap(iconPath).scaledToHeight(15));
      label->setVisible(false);
      label->setToolTip(toolTip);

      layout()->addWidget(label);
      m_labels[info] = label;
    }

    auto label = new QLabel("*");
    auto palette = label->palette();
    palette.setColor(QPalette::WindowText, Qt::darkRed);
    label->setPalette(palette);
    label->setVisible(false);
    layout()->addWidget(label);
    m_labels[INVALID] = label;
  }

  /**
   * Set the visibility of the given information.
   *
   * @param info    :: info to set
   * @param visible :: whether or not the info should be visible
   */
  void PropertyInfoWidget::setInfoVisible( Info info, bool visible )
  {
    m_labels[info]->setVisible(visible);
  }

  /**
   * Set the tool tip of the given information.
   *
   * @param info    :: info to set
   * @param toolTip :: the contents of the tool tip
   */
  void PropertyInfoWidget::setInfoToolTip( Info info, const QString & toolTip )
  {
    m_labels[info]->setToolTip(toolTip);
  }

}//namespace
}//namespace
