#include "MenuWithToolTips.h"

#include <QEvent>
#include <QHelpEvent>
#include <QToolTip>

/**
 * @param parent An optional parent. Default=nullptr
 */
MenuWithToolTips::MenuWithToolTips(QWidget *parent) : QMenu(parent) {}

/**
 * @param title Title text for the menu
 * @param parent An optional parent. Default=nullptr
 */
MenuWithToolTips::MenuWithToolTips(const QString &title, QWidget *parent)
    : QMenu(title, parent) {}

/**
 * Override standard QMenu behaviour to show the tooltip when requested
 * @param e A pointer to the event being processed
 */
bool MenuWithToolTips::event(QEvent *e) {
  if (e->type() == QEvent::ToolTip && activeAction() != 0) {
    const QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
    QToolTip::showText(helpEvent->globalPos(), activeAction()->toolTip());
  } else {
    QToolTip::hideText();
  }
  return QMenu::event(e);
}
