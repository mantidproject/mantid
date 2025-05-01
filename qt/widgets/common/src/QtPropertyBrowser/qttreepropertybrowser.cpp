/****************************************************************************
**
** This file is part of a Qt Solutions component.
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Solutions Commercial License Agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** Please note Third Party Software included with Qt Solutions may impose
** additional restrictions and it is the user's responsibility to ensure
** that they have met the licensing requirements of the GPL, LGPL, or Qt
** Solutions Commercial license and the relevant license of the Third
** Party Software they are using.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"
#include <QApplication>
#include <QCheckBox>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemDelegate>
#include <QLineEdit>
#include <QPainter>
#include <QPalette>
#include <QSet>
#include <QStyle>
#include <QTreeWidget>

namespace {
// Translation function for Qt5
QString translateUtf8Encoded(const char *context, const char *key, const char *disambiguation = nullptr, int n = -1) {
  return QApplication::translate(context, key, disambiguation, n);
}
} // namespace

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QtPropertyEditorView;

// ------------ QtPropertyEditorView

QtPropertyEditorView::QtPropertyEditorView(QWidget *parent, bool darkTopLevel)
    : QTreeWidget(parent), m_editorPrivate(nullptr), m_darkTopLevel(darkTopLevel) {
  connect(header(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(resizeColumnToContents(int)));
}

void QtPropertyEditorView::drawRow(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const {
  QStyleOptionViewItem opt = option;

  bool hasValue = true;
  if (m_editorPrivate) {
    const QtProperty *property = m_editorPrivate->indexToProperty(index);
    if (property)
      hasValue = property->hasValue();
  }
  if (!hasValue && m_editorPrivate->markPropertiesWithoutValue()) {
    const QColor c = option.palette.color(QPalette::Dark);
    painter->fillRect(option.rect, c);
    opt.palette.setColor(QPalette::AlternateBase, c);
  } else {
    QColor c = m_editorPrivate->calculatedBackgroundColor(m_editorPrivate->indexToBrowserItem(index));
    if (index.parent() == QModelIndex() && m_darkTopLevel) {
      c = option.palette.color(QPalette::Mid);
    }
    if (c.isValid()) {

      painter->fillRect(option.rect, c);
      opt.palette.setColor(QPalette::AlternateBase, c.lighter(112));
    }
  }
  QTreeWidget::drawRow(painter, opt, index);
  QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
  painter->save();
  painter->setPen(QPen(color));
  painter->drawLine(opt.rect.x(), opt.rect.bottom(), opt.rect.right(), opt.rect.bottom());
  painter->restore();
}

void QtPropertyEditorView::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_Return:
  case Qt::Key_Enter:
  case Qt::Key_Space: // Trigger Edit
    if (!m_editorPrivate->editedItem())
      if (const QTreeWidgetItem *item = currentItem())
        if (item->columnCount() >= 2 &&
            ((item->flags() & (Qt::ItemIsEditable | Qt::ItemIsEnabled)) == (Qt::ItemIsEditable | Qt::ItemIsEnabled))) {
          event->accept();
          // If the current position is at column 0, move to 1.
          QModelIndex index = currentIndex();
          if (index.column() == 0) {
            index = index.sibling(index.row(), 1);
            setCurrentIndex(index);
          }
          edit(index);
          return;
        }
    break;
  default:
    break;
  }
  QTreeWidget::keyPressEvent(event);
}

void QtPropertyEditorView::mousePressEvent(QMouseEvent *event) {
  QTreeWidget::mousePressEvent(event);
  QTreeWidgetItem *item = itemAt(event->pos());
  auto index = currentIndex();

  if (item) {
    if ((item != m_editorPrivate->editedItem()) && (event->button() == Qt::LeftButton) &&
        (header()->logicalIndexAt(event->pos().x()) == 1) &&
        ((item->flags() & (Qt::ItemIsEditable | Qt::ItemIsEnabled)) == (Qt::ItemIsEditable | Qt::ItemIsEnabled))) {
      editItem(item, 1);
    } else if (!m_editorPrivate->hasValue(item) && m_editorPrivate->markPropertiesWithoutValue() &&
               !rootIsDecorated()) {
      if (event->pos().x() + header()->offset() < 20)
        item->setExpanded(!item->isExpanded());
    } else if (index.column() == 2) {
      editItem(item, 2);
    }
  }
}

// ------------ QtPropertyEditorDelegate

int QtPropertyEditorDelegate::indentation(const QModelIndex &index) const {
  if (!m_editorPrivate)
    return 0;

  QTreeWidgetItem *item = m_editorPrivate->indexToItem(index);
  int indent = 0;
  while (item->parent()) {
    item = item->parent();
    ++indent;
  }
  if (m_editorPrivate->treeWidget()->rootIsDecorated())
    ++indent;
  return indent * m_editorPrivate->treeWidget()->indentation();
}

void QtPropertyEditorDelegate::slotEditorDestroyed(QObject *object) {
  if (auto w = qobject_cast<const QWidget *>(object)) {
    const EditorToPropertyMap::iterator it = m_editorToProperty.find(w);
    if (it != m_editorToProperty.end()) {
      m_propertyToEditor.remove(it.value());
      m_editorToProperty.erase(it);
    }
    if (m_editedWidget == w) {
      m_editedWidget = nullptr;
      m_editedItem = nullptr;
    }
  }
}

void QtPropertyEditorDelegate::closeEditor(QtProperty *property) {
  if (QWidget *w = m_propertyToEditor.value(property, 0))
    w->deleteLater();
}

QWidget *QtPropertyEditorDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                                                const QModelIndex &index) const {
  if (index.column() == 1 && m_editorPrivate) {
    QtProperty *property = m_editorPrivate->indexToProperty(index);
    QTreeWidgetItem *item = m_editorPrivate->indexToItem(index);
    if (property && item && (item->flags() & Qt::ItemIsEnabled)) {
      QWidget *editor = m_editorPrivate->createEditor(property, parent);
      if (editor) {
        editor->setAutoFillBackground(true);
        editor->installEventFilter(const_cast<QtPropertyEditorDelegate *>(this));
        connect(editor, SIGNAL(destroyed(QObject *)), this, SLOT(slotEditorDestroyed(QObject *)));
        m_propertyToEditor[property] = editor;
        m_editorToProperty[editor] = property;
        m_editedItem = item;
        m_editedWidget = editor;
      }
      return editor;
    }
  }
  if (index.column() > 1 && m_editorPrivate) {
    QtProperty *property = m_editorPrivate->indexToProperty(index);
    int optionIndex = index.column() - 2;
    if (optionIndex >= m_editorPrivate->options().size()) {
      return nullptr;
    }
    QString optionName = m_editorPrivate->options()[optionIndex];
    if (property->hasOption(optionName)) {
      QWidget *editor = new PropertyOptionCheckBox(parent, property, optionName);
      connect(editor, SIGNAL(optionChanged(QtProperty *, const QString &, bool)), this,
              SIGNAL(optionChanged(QtProperty *, const QString &, bool)));
      return editor;
    }
  }
  return nullptr;
}

void QtPropertyEditorDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                                    const QModelIndex &index) const {
  Q_UNUSED(index)
  editor->setGeometry(option.rect.adjusted(0, 0, 0, -1));
}

void QtPropertyEditorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
  bool hasValue = true;
  if (m_editorPrivate) {
    const QtProperty *property = m_editorPrivate->indexToProperty(index);
    if (property)
      hasValue = property->hasValue();
  }

  QStyleOptionViewItem opt = option;
  if ((m_editorPrivate && index.column() == 0) || !hasValue) {
    const QtProperty *property = m_editorPrivate->indexToProperty(index);
    if (property && property->isModified()) {
      opt.font.setBold(true);
      opt.fontMetrics = QFontMetrics(opt.font);
    }
  }
  QColor c;
  if (!hasValue && m_editorPrivate->markPropertiesWithoutValue()) {
    c = opt.palette.color(QPalette::Dark);
    opt.palette.setColor(QPalette::Text, opt.palette.color(QPalette::BrightText));
  } else {
    c = m_editorPrivate->calculatedBackgroundColor(m_editorPrivate->indexToBrowserItem(index));
    if (c.isValid() && (opt.features & QStyleOptionViewItem::Alternate))
      c = c.lighter(112);
  }
  if (c.isValid())
    painter->fillRect(option.rect, c);
  opt.state &= ~QStyle::State_HasFocus;
  QItemDelegate::paint(painter, opt, index);

  opt.palette.setCurrentColorGroup(QPalette::Active);
  QColor color = static_cast<QRgb>(QApplication::style()->styleHint(QStyle::SH_Table_GridLineColor, &opt));
  painter->save();
  painter->setPen(QPen(color));
  if (!m_editorPrivate || (!m_editorPrivate->lastColumn(index.column()) && hasValue)) {
    int right = (option.direction == Qt::LeftToRight) ? option.rect.right() : option.rect.left();
    painter->drawLine(right, option.rect.y(), right, option.rect.bottom());
  }
  painter->restore();
  if (index.column() > 1) {
    const QtProperty *property = m_editorPrivate->indexToProperty(index);
    int optionIndex = index.column() - 2;
    if (optionIndex >= m_editorPrivate->options().size()) {
      return;
    }
    QString optionName = m_editorPrivate->options()[optionIndex];
    if (property->hasOption(optionName)) {
      QStyleOptionButton optButton;
      auto state = property->checkOption(optionName) ? QStyle::State_On : QStyle::State_Off;
      optButton.state |= state;
      optButton.rect = option.rect;
      optButton.rect.setWidth(optButton.rect.height());
      QApplication::style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &optButton, painter);
    }
  }
}

QSize QtPropertyEditorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
  return QItemDelegate::sizeHint(option, index) + QSize(3, 4);
}

bool QtPropertyEditorDelegate::eventFilter(QObject *object, QEvent *event) {
  if (event->type() == QEvent::FocusOut) {
    auto *fe = static_cast<QFocusEvent *>(event);
    if (fe->reason() == Qt::ActiveWindowFocusReason)
      return false;
  }
  return QItemDelegate::eventFilter(object, event);
}

//  -------- QtTreePropertyBrowserPrivate implementation
QtTreePropertyBrowserPrivate::QtTreePropertyBrowserPrivate()
    : m_treeWidget(nullptr), m_headerVisible(true), m_resizeMode(QtTreePropertyBrowser::Stretch), m_delegate(nullptr),
      m_markPropertiesWithoutValue(false), m_browserChangedBlocked(false) {}

// Draw an icon indicating opened/closing branches
static QIcon drawIndicatorIcon(const QPalette &palette, QStyle *style) {
  QPixmap pix(14, 14);
  pix.fill(Qt::transparent);
  QStyleOption branchOption;
  branchOption.rect = QRect(2, 2, 9, 9); // ### hardcoded in qcommonstyle.cpp
  branchOption.palette = palette;
  branchOption.state = QStyle::State_Children;

  QPainter p;
  // Draw closed state
  p.begin(&pix);
  style->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, &p);
  p.end();
  QIcon rc = pix;
  rc.addPixmap(pix, QIcon::Selected, QIcon::Off);
  // Draw opened state
  branchOption.state |= QStyle::State_Open;
  pix.fill(Qt::transparent);
  p.begin(&pix);
  style->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOption, &p);
  p.end();

  rc.addPixmap(pix, QIcon::Normal, QIcon::On);
  rc.addPixmap(pix, QIcon::Selected, QIcon::On);
  return rc;
}

void QtTreePropertyBrowserPrivate::init(QWidget *parent, const QStringList &options, bool darkTopLevel) {
  auto *layout = new QHBoxLayout(parent);
  layout->setMargin(0);
  m_treeWidget = new QtPropertyEditorView(parent, darkTopLevel);
  m_treeWidget->setEditorPrivate(this);
  m_treeWidget->setIconSize(QSize(18, 18));
  layout->addWidget(m_treeWidget);

  m_options = options;
  const int columnCount = 2 + m_options.size();
  m_treeWidget->setColumnCount(columnCount);
  QStringList labels;
  labels.append(translateUtf8Encoded("QtTreePropertyBrowser", "Property", nullptr));
  labels.append(QApplication::translate("QtTreePropertyBrowser", "Value", nullptr));
  // add optional columns
  foreach (const auto &opt, m_options) {
    labels.append(QApplication::translate("QtTreePropertyBrowser", opt.toStdString().c_str(), nullptr));
  }
  m_treeWidget->setHeaderLabels(labels);
  m_treeWidget->setAlternatingRowColors(true);
  m_treeWidget->setEditTriggers(QAbstractItemView::EditKeyPressed);
  m_delegate = new QtPropertyEditorDelegate(parent);
  m_delegate->setEditorPrivate(this);
  QObject::connect(m_delegate, SIGNAL(optionChanged(QtProperty *, const QString &, bool)), parent,
                   SIGNAL(optionChanged(QtProperty *, const QString &, bool)));
  m_treeWidget->setItemDelegate(m_delegate);
  m_treeWidget->header()->setSectionsMovable(false);
  m_treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);

  m_expandIcon = drawIndicatorIcon(q_ptr->palette(), q_ptr->style());

  QObject::connect(m_treeWidget, SIGNAL(collapsed(const QModelIndex &)), q_ptr,
                   SLOT(slotCollapsed(const QModelIndex &)));
  QObject::connect(m_treeWidget, SIGNAL(expanded(const QModelIndex &)), q_ptr, SLOT(slotExpanded(const QModelIndex &)));
  QObject::connect(m_treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), q_ptr,
                   SLOT(slotCurrentTreeItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
}

QtBrowserItem *QtTreePropertyBrowserPrivate::currentItem() const {
  if (QTreeWidgetItem *treeItem = m_treeWidget->currentItem())
    return m_itemToIndex.value(treeItem);
  return nullptr;
}

void QtTreePropertyBrowserPrivate::setCurrentItem(QtBrowserItem *browserItem, bool block) {
  const bool blocked = block ? m_treeWidget->blockSignals(true) : false;
  if (browserItem == nullptr)
    m_treeWidget->setCurrentItem(nullptr);
  else
    m_treeWidget->setCurrentItem(m_indexToItem.value(browserItem));
  if (block)
    m_treeWidget->blockSignals(blocked);
}

QtProperty *QtTreePropertyBrowserPrivate::indexToProperty(const QModelIndex &index) const {
  QTreeWidgetItem *item = m_treeWidget->indexToItem(index);
  const auto idx = m_itemToIndex.value(item);
  if (idx)
    return idx->property();
  return nullptr;
}

QtBrowserItem *QtTreePropertyBrowserPrivate::indexToBrowserItem(const QModelIndex &index) const {
  QTreeWidgetItem *item = m_treeWidget->indexToItem(index);
  return m_itemToIndex.value(item);
}

QTreeWidgetItem *QtTreePropertyBrowserPrivate::indexToItem(const QModelIndex &index) const {
  return m_treeWidget->indexToItem(index);
}

bool QtTreePropertyBrowserPrivate::lastColumn(int column) const {
  return m_treeWidget->header()->visualIndex(column) == m_treeWidget->columnCount() - 1;
}

void QtTreePropertyBrowserPrivate::disableItem(QTreeWidgetItem *item) const {
  Qt::ItemFlags flags = item->flags();
  if (flags & Qt::ItemIsEnabled) {
    flags &= ~Qt::ItemIsEnabled;
    item->setFlags(flags);
    m_delegate->closeEditor(m_itemToIndex[item]->property());
    const int childCount = item->childCount();
    for (int i = 0; i < childCount; i++) {
      QTreeWidgetItem *child = item->child(i);
      disableItem(child);
    }
  }
}

void QtTreePropertyBrowserPrivate::enableItem(QTreeWidgetItem *item) const {
  Qt::ItemFlags flags = item->flags();
  flags |= Qt::ItemIsEnabled;
  item->setFlags(flags);
  const int childCount = item->childCount();
  for (int i = 0; i < childCount; i++) {
    QTreeWidgetItem *child = item->child(i);
    const QtProperty *property = m_itemToIndex[child]->property();
    if (property->isEnabled()) {
      enableItem(child);
    }
  }
}

bool QtTreePropertyBrowserPrivate::hasValue(QTreeWidgetItem *item) const {
  const auto browserItem = m_itemToIndex.value(item);
  if (browserItem)
    return browserItem->property()->hasValue();
  return false;
}

void QtTreePropertyBrowserPrivate::propertyInserted(QtBrowserItem *index, QtBrowserItem *afterIndex) {
  QTreeWidgetItem *afterItem = m_indexToItem.value(afterIndex);
  QTreeWidgetItem *parentItem = m_indexToItem.value(index->parent());

  QTreeWidgetItem *newItem = nullptr;
  if (parentItem) {
    newItem = new QTreeWidgetItem(parentItem, afterItem);
  } else {
    newItem = new QTreeWidgetItem(m_treeWidget, afterItem);
  }
  m_itemToIndex[newItem] = index;
  m_indexToItem[index] = newItem;

  newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
  newItem->setExpanded(true);

  updateItem(newItem);
}

void QtTreePropertyBrowserPrivate::propertyRemoved(QtBrowserItem *index) {
  QTreeWidgetItem *item = m_indexToItem.value(index);

  if (m_treeWidget->currentItem() == item) {
    m_treeWidget->setCurrentItem(nullptr);
  }

  delete item;

  m_indexToItem.remove(index);
  m_itemToIndex.remove(item);
  m_indexToBackgroundColor.remove(index);
}

void QtTreePropertyBrowserPrivate::propertyChanged(QtBrowserItem *index) {
  QTreeWidgetItem *item = m_indexToItem.value(index);

  updateItem(item);
}

void QtTreePropertyBrowserPrivate::updateItem(QTreeWidgetItem *item) {
  const QtProperty *property = m_itemToIndex[item]->property();
  QString toolTip = property->toolTip();
  QIcon expandIcon;
  if (property->hasValue()) {
    if (toolTip.isEmpty())
      toolTip = property->valueText();

    // If property has value, use custom tooltip for value and default one for
    // name
    item->setToolTip(1, toolTip);
    item->setToolTip(0, property->propertyName());

    item->setIcon(1, property->valueIcon());
    item->setText(1, property->valueText());
  } else {
    if (toolTip.isEmpty())
      toolTip = property->propertyName();

    // If property doesn't have value, use custom tooltip for it
    item->setToolTip(0, toolTip);

    if (markPropertiesWithoutValue() && !m_treeWidget->rootIsDecorated())
      expandIcon = m_expandIcon;
  }
  item->setIcon(0, expandIcon);
  item->setFirstColumnSpanned(!property->hasValue());
  item->setStatusTip(0, property->statusTip());
  item->setWhatsThis(0, property->whatsThis());
  item->setText(0, property->propertyName());
  bool wasEnabled = item->flags() & Qt::ItemIsEnabled;
  bool isEnabled = property->isEnabled() ? !item->parent() || (item->parent()->flags() & Qt::ItemIsEnabled) : false;

  if (wasEnabled != isEnabled) {
    if (isEnabled)
      enableItem(item);
    else
      disableItem(item);
  }
  m_treeWidget->viewport()->update();
}

QColor QtTreePropertyBrowserPrivate::calculatedBackgroundColor(QtBrowserItem *item) const {
  const auto it = m_indexToBackgroundColor.constFind(item);
  if (it != m_indexToBackgroundColor.cend()) {
    return it.value();
  } else if (item->parent()) {
    return calculatedBackgroundColor(item->parent());
  }

  return QColor();
}

void QtTreePropertyBrowserPrivate::slotCollapsed(const QModelIndex &index) {
  QTreeWidgetItem *item = indexToItem(index);
  QtBrowserItem *idx = m_itemToIndex.value(item);
  if (item)
    emit q_ptr->collapsed(idx);
}

void QtTreePropertyBrowserPrivate::slotExpanded(const QModelIndex &index) {
  QTreeWidgetItem *item = indexToItem(index);
  QtBrowserItem *idx = m_itemToIndex.value(item);
  if (item)
    emit q_ptr->expanded(idx);
}

void QtTreePropertyBrowserPrivate::slotCurrentBrowserItemChanged(QtBrowserItem *item) {
  if (!m_browserChangedBlocked && item != currentItem())
    setCurrentItem(item, true);
}

void QtTreePropertyBrowserPrivate::slotCurrentTreeItemChanged(QTreeWidgetItem *newItem, QTreeWidgetItem * /*unused*/) {
  QtBrowserItem *browserItem = newItem ? m_itemToIndex.value(newItem) : 0;
  m_browserChangedBlocked = true;
  q_ptr->setCurrentItem(browserItem);
  m_browserChangedBlocked = false;
}

QTreeWidgetItem *QtTreePropertyBrowserPrivate::editedItem() const { return m_delegate->editedItem(); }

void QtTreePropertyBrowserPrivate::closeEditor() {
  auto treeItem = editedItem();
  const QtBrowserItem *browserItem = m_itemToIndex[treeItem];
  m_delegate->closeEditor(browserItem->property());
}

void QtTreePropertyBrowserPrivate::editItem(QtBrowserItem *browserItem) {
  if (QTreeWidgetItem *treeItem = m_indexToItem.value(browserItem, 0)) {
    m_treeWidget->setCurrentItem(treeItem, 1);
    m_treeWidget->editItem(treeItem, 1);
  }
}

QTreeWidgetItem *QtTreePropertyBrowserPrivate::getItemWidget(QtBrowserItem *browserItem) {
  return m_indexToItem.value(browserItem, 0);
}

void QtTreePropertyBrowserPrivate::disableItem(QtBrowserItem *browserItem) {
  if (QTreeWidgetItem *treeItem = m_indexToItem.value(browserItem, 0)) {
    disableItem(treeItem);
  }
}

void QtTreePropertyBrowserPrivate::setColumnSizes(int s0, int s1, int s2) {
  m_treeWidget->header()->setSectionResizeMode(QHeaderView::Interactive);
  m_treeWidget->header()->setStretchLastSection(false);
  m_treeWidget->header()->resizeSection(0, s0);
  m_treeWidget->header()->resizeSection(1, s1);
  if (!m_options.isEmpty()) {
    if (s2 < 0)
      s2 = s1;
    m_treeWidget->header()->resizeSection(2, s2);
  }
}

void QtTreePropertyBrowserPrivate::setStretchLastColumn(bool stretch) {
  m_treeWidget->header()->setStretchLastSection(stretch);
}

void QtTreePropertyBrowserPrivate::hideColumn(int col) { m_treeWidget->header()->hideSection(col); }

void QtTreePropertyBrowserPrivate::showColumn(int col) { m_treeWidget->header()->showSection(col); }

/**
    \class QtTreePropertyBrowser

    \brief The QtTreePropertyBrowser class provides QTreeWidget based
    property browser.

    A property browser is a widget that enables the user to edit a
    given set of properties. Each property is represented by a label
    specifying the property's name, and an editing widget (e.g. a line
    edit or a combobox) holding its value. A property can have zero or
    more subproperties.

    QtTreePropertyBrowser provides a tree based view for all nested
    properties, i.e. properties that have subproperties can be in an
    expanded (subproperties are visible) or collapsed (subproperties
    are hidden) state. For example:

    \image qttreepropertybrowser.png

    Use the QtAbstractPropertyBrowser API to add, insert and remove
    properties from an instance of the QtTreePropertyBrowser class.
    The properties themselves are created and managed by
    implementations of the QtAbstractPropertyManager class.

    \sa QtGroupBoxPropertyBrowser, QtAbstractPropertyBrowser
*/

/**
    \fn void QtTreePropertyBrowser::collapsed(QtBrowserItem *item)

    This signal is emitted when the \a item is collapsed.

    \sa expanded(), setExpanded()
*/

/**
    \fn void QtTreePropertyBrowser::expanded(QtBrowserItem *item)

    This signal is emitted when the \a item is expanded.

    \sa collapsed(), setExpanded()
*/

/**
    Creates a property browser with the given \a parent.
*/
QtTreePropertyBrowser::QtTreePropertyBrowser(QWidget *parent, const QStringList &options, bool darkTopLevel)
    : QtAbstractPropertyBrowser(parent) {
  d_ptr = new QtTreePropertyBrowserPrivate;
  d_ptr->q_ptr = this;

  d_ptr->init(this, options, darkTopLevel);
  connect(this, SIGNAL(currentItemChanged(QtBrowserItem *)), this,
          SLOT(slotCurrentBrowserItemChanged(QtBrowserItem *)));
}

/**
    Destroys this property browser.

    Note that the properties that were inserted into this browser are
    \e not destroyed since they may still be used in other
    browsers. The properties are owned by the manager that created
    them.

    \sa QtProperty, QtAbstractPropertyManager
*/
QtTreePropertyBrowser::~QtTreePropertyBrowser() { delete d_ptr; }

/**
    \property QtTreePropertyBrowser::indentation
    \brief indentation of the items in the tree view.
*/
int QtTreePropertyBrowser::indentation() const { return d_ptr->m_treeWidget->indentation(); }

void QtTreePropertyBrowser::setIndentation(int i) { d_ptr->m_treeWidget->setIndentation(i); }

/**
  \property QtTreePropertyBrowser::rootIsDecorated
  \brief whether to show controls for expanding and collapsing root items.
*/
bool QtTreePropertyBrowser::rootIsDecorated() const { return d_ptr->m_treeWidget->rootIsDecorated(); }

void QtTreePropertyBrowser::setRootIsDecorated(bool show) {
  d_ptr->m_treeWidget->setRootIsDecorated(show);
  QMapIterator<QTreeWidgetItem *, QtBrowserItem *> it(d_ptr->m_itemToIndex);
  while (it.hasNext()) {
    const auto property = it.next().value()->property();
    if (!property->hasValue())
      d_ptr->updateItem(it.key());
  }
}

/**
  \property QtTreePropertyBrowser::alternatingRowColors
  \brief whether to draw the background using alternating colors.
  By default this property is set to true.
*/
bool QtTreePropertyBrowser::alternatingRowColors() const { return d_ptr->m_treeWidget->alternatingRowColors(); }

void QtTreePropertyBrowser::setAlternatingRowColors(bool enable) {
  d_ptr->m_treeWidget->setAlternatingRowColors(enable);
  QMapIterator<QTreeWidgetItem *, QtBrowserItem *> it(d_ptr->m_itemToIndex);
}

/**
  \property QtTreePropertyBrowser::headerVisible
  \brief whether to show the header.
*/
bool QtTreePropertyBrowser::isHeaderVisible() const { return d_ptr->m_headerVisible; }

void QtTreePropertyBrowser::setHeaderVisible(bool visible) {
  if (d_ptr->m_headerVisible == visible)
    return;

  d_ptr->m_headerVisible = visible;
  d_ptr->m_treeWidget->header()->setVisible(visible);
}

/**
  \enum QtTreePropertyBrowser::ResizeMode

  The resize mode specifies the behavior of the header sections.

  \value Interactive The user can resize the sections.
  The sections can also be resized programmatically using setSplitterPosition().

  \value Fixed The user cannot resize the section.
  The section can only be resized programmatically using setSplitterPosition().

  \value Stretch QHeaderView will automatically resize the section to fill the
  available space.
  The size cannot be changed by the user or programmatically.

  \value ResizeToContents QHeaderView will automatically resize the section to
  its optimal
  size based on the contents of the entire column.
  The size cannot be changed by the user or programmatically.

  \sa setResizeMode()
*/

/**
    \property QtTreePropertyBrowser::resizeMode
    \brief the resize mode of setions in the header.
*/

QtTreePropertyBrowser::ResizeMode QtTreePropertyBrowser::resizeMode() const { return d_ptr->m_resizeMode; }

void QtTreePropertyBrowser::setResizeMode(QtTreePropertyBrowser::ResizeMode mode) {
  if (d_ptr->m_resizeMode == mode)
    return;

  d_ptr->m_resizeMode = mode;
  QHeaderView::ResizeMode m = QHeaderView::Stretch;
  switch (mode) {
  case QtTreePropertyBrowser::Interactive:
    m = QHeaderView::Interactive;
    break;
  case QtTreePropertyBrowser::Fixed:
    m = QHeaderView::Fixed;
    break;
  case QtTreePropertyBrowser::ResizeToContents:
    m = QHeaderView::ResizeToContents;
    break;
  case QtTreePropertyBrowser::Stretch:
  default:
    m = QHeaderView::Stretch;
    break;
  }
  d_ptr->m_treeWidget->header()->setSectionResizeMode(m);
}

/**
    \property QtTreePropertyBrowser::splitterPosition
    \brief the position of the splitter between the colunms.
*/

int QtTreePropertyBrowser::splitterPosition() const { return d_ptr->m_treeWidget->header()->sectionSize(0); }

void QtTreePropertyBrowser::setSplitterPosition(int position) {
  d_ptr->m_treeWidget->header()->resizeSection(0, position);
}

/**
    Sets the \a item to either collapse or expanded, depending on the value of
   \a expanded.

    \sa isExpanded(), expanded(), collapsed()
*/

void QtTreePropertyBrowser::setExpanded(QtBrowserItem *item, bool expanded) {
  QTreeWidgetItem *treeItem = d_ptr->m_indexToItem.value(item);
  if (treeItem)
    treeItem->setExpanded(expanded);
}

/**
    Returns true if the \a item is expanded; otherwise returns false.

    \sa setExpanded()
*/

bool QtTreePropertyBrowser::isExpanded(QtBrowserItem *item) const {
  QTreeWidgetItem *treeItem = d_ptr->m_indexToItem.value(item);
  if (treeItem)
    return treeItem->isExpanded();
  return false;
}

/**
    Returns true if the \a item is visible; otherwise returns false.

    \sa setItemVisible()
    \since 4.5
*/

bool QtTreePropertyBrowser::isItemVisible(QtBrowserItem *item) const {
  if (const QTreeWidgetItem *treeItem = d_ptr->m_indexToItem.value(item))
    return !treeItem->isHidden();
  return false;
}

/**
    Sets the \a item to be visible, depending on the value of \a visible.

   \sa isItemVisible()
   \since 4.5
*/

void QtTreePropertyBrowser::setItemVisible(QtBrowserItem *item, bool visible) {
  if (QTreeWidgetItem *treeItem = d_ptr->m_indexToItem.value(item))
    treeItem->setHidden(!visible);
}

/**
    Sets the \a item's background color to \a color. Note that while item's
   background
    is rendered every second row is being drawn with alternate color (which is a
   bit lighter than items \a color)

    \sa backgroundColor(), calculatedBackgroundColor()
*/

void QtTreePropertyBrowser::setBackgroundColor(QtBrowserItem *item, const QColor &color) {
  if (!d_ptr->m_indexToItem.contains(item))
    return;
  if (color.isValid())
    d_ptr->m_indexToBackgroundColor[item] = color;
  else
    d_ptr->m_indexToBackgroundColor.remove(item);
  d_ptr->m_treeWidget->viewport()->update();
}

/**
    Returns the \a item's color. If there is no color set for item it returns
   invalid color.

    \sa calculatedBackgroundColor(), setBackgroundColor()
*/

QColor QtTreePropertyBrowser::backgroundColor(QtBrowserItem *item) const {
  return d_ptr->m_indexToBackgroundColor.value(item);
}

/**
    Returns the \a item's color. If there is no color set for item it returns
   parent \a item's
    color (if there is no color set for parent it returns grandparent's color
   and so on). In case
    the color is not set for \a item and it's top level item it returns invalid
   color.

    \sa backgroundColor(), setBackgroundColor()
*/

QColor QtTreePropertyBrowser::calculatedBackgroundColor(QtBrowserItem *item) const {
  return d_ptr->calculatedBackgroundColor(item);
}

/**
    \property QtTreePropertyBrowser::propertiesWithoutValueMarked
    \brief whether to enable or disable marking properties without value.

    When marking is enabled the item's background is rendered in dark color and
   item's
    foreground is rendered with light color.

    \sa propertiesWithoutValueMarked()
*/
void QtTreePropertyBrowser::setPropertiesWithoutValueMarked(bool mark) {
  if (d_ptr->m_markPropertiesWithoutValue == mark)
    return;

  d_ptr->m_markPropertiesWithoutValue = mark;
  QMapIterator<QTreeWidgetItem *, QtBrowserItem *> it(d_ptr->m_itemToIndex);
  while (it.hasNext()) {
    const auto property = it.next().value()->property();
    if (!property->hasValue())
      d_ptr->updateItem(it.key());
  }
  d_ptr->m_treeWidget->viewport()->update();
}

bool QtTreePropertyBrowser::propertiesWithoutValueMarked() const { return d_ptr->m_markPropertiesWithoutValue; }

/**
    \reimp
*/
void QtTreePropertyBrowser::itemInserted(QtBrowserItem *item, QtBrowserItem *afterItem) {
  d_ptr->propertyInserted(item, afterItem);
}

/**
    \reimp
*/
void QtTreePropertyBrowser::itemRemoved(QtBrowserItem *item) { d_ptr->propertyRemoved(item); }

/**
    \reimp
*/
void QtTreePropertyBrowser::itemChanged(QtBrowserItem *item) { d_ptr->propertyChanged(item); }

/**
    Sets the current item to \a item and opens the relevant editor for it.
*/
void QtTreePropertyBrowser::editItem(QtBrowserItem *item) { d_ptr->editItem(item); }

void QtTreePropertyBrowser::setColumnSizes(int s0, int s1, int s2) { d_ptr->setColumnSizes(s0, s1, s2); }

void QtTreePropertyBrowser::setStretchLastColumn(bool stretch) { d_ptr->setStretchLastColumn(stretch); }

void QtTreePropertyBrowser::hideColumn(int col) { d_ptr->hideColumn(col); }

void QtTreePropertyBrowser::showColumn(int col) { d_ptr->showColumn(col); }

QTreeWidgetItem *QtTreePropertyBrowser::getItemWidget(QtBrowserItem *item) { return d_ptr->getItemWidget(item); }

QTreeWidget *QtTreePropertyBrowser::treeWidget() { return d_ptr->treeWidget(); }

void QtTreePropertyBrowser::closeEditor() { d_ptr->closeEditor(); }

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif
