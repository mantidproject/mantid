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

#pragma once

#include "qtpropertybrowser.h"

#include <QApplication>
#include <QHeaderView>
#include <QItemDelegate>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionButton>
#include <QTreeWidget>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QTreeWidgetItem;
class QtTreePropertyBrowserPrivate;

class EXPORT_OPT_MANTIDQT_COMMON QtTreePropertyBrowser : public QtAbstractPropertyBrowser {
  Q_OBJECT
  Q_ENUMS(ResizeMode)
  Q_PROPERTY(int indentation READ indentation WRITE setIndentation)
  Q_PROPERTY(bool rootIsDecorated READ rootIsDecorated WRITE setRootIsDecorated)
  Q_PROPERTY(bool alternatingRowColors READ alternatingRowColors WRITE setAlternatingRowColors)
  Q_PROPERTY(bool headerVisible READ isHeaderVisible WRITE setHeaderVisible)
  Q_PROPERTY(ResizeMode resizeMode READ resizeMode WRITE setResizeMode)
  Q_PROPERTY(int splitterPosition READ splitterPosition WRITE setSplitterPosition)
  Q_PROPERTY(bool propertiesWithoutValueMarked READ propertiesWithoutValueMarked WRITE setPropertiesWithoutValueMarked)
public:
  enum ResizeMode { Interactive, Stretch, Fixed, ResizeToContents };

  QtTreePropertyBrowser(QWidget *parent = nullptr, const QStringList &options = QStringList(),
                        bool darkTopLevel = true);
  ~QtTreePropertyBrowser() override;

  int indentation() const;
  void setIndentation(int i);

  bool rootIsDecorated() const;
  void setRootIsDecorated(bool show);

  bool alternatingRowColors() const;
  void setAlternatingRowColors(bool enable);

  bool isHeaderVisible() const;
  void setHeaderVisible(bool visible);

  ResizeMode resizeMode() const;
  void setResizeMode(ResizeMode mode);

  int splitterPosition() const;
  void setSplitterPosition(int position);

  void setExpanded(QtBrowserItem *item, bool expanded);
  bool isExpanded(QtBrowserItem *item) const;

  bool isItemVisible(QtBrowserItem *item) const;
  void setItemVisible(QtBrowserItem *item, bool visible);

  void hideColumn(int col);
  void showColumn(int col);

  void setBackgroundColor(QtBrowserItem *item, const QColor &color);
  QColor backgroundColor(QtBrowserItem *item) const;
  QColor calculatedBackgroundColor(QtBrowserItem *item) const;

  void setPropertiesWithoutValueMarked(bool mark);
  bool propertiesWithoutValueMarked() const;

  void editItem(QtBrowserItem *item);
  void setColumnSizes(int s0, int s1, int s2 = -1);
  void setStretchLastColumn(bool stretch);

  QTreeWidgetItem *getItemWidget(QtBrowserItem *item);
  QTreeWidget *treeWidget();

Q_SIGNALS:

  void collapsed(QtBrowserItem *item);
  void expanded(QtBrowserItem *item);
  void optionChanged(QtProperty * /*_t1*/, const QString & /*_t2*/, bool /*_t3*/);

public Q_SLOTS:

  void closeEditor();

protected:
  void itemInserted(QtBrowserItem *item, QtBrowserItem *afterItem) override;
  void itemRemoved(QtBrowserItem *item) override;
  void itemChanged(QtBrowserItem *item) override;

private:
  QtTreePropertyBrowserPrivate *d_ptr;
  Q_DECLARE_PRIVATE(QtTreePropertyBrowser)
  Q_DISABLE_COPY(QtTreePropertyBrowser)

  Q_PRIVATE_SLOT(d_func(), void slotCollapsed(const QModelIndex &))
  Q_PRIVATE_SLOT(d_func(), void slotExpanded(const QModelIndex &))
  Q_PRIVATE_SLOT(d_func(), void slotCurrentBrowserItemChanged(QtBrowserItem *))
  Q_PRIVATE_SLOT(d_func(), void slotCurrentTreeItemChanged(QTreeWidgetItem *, QTreeWidgetItem *))
};

class PropertyOptionCheckBox : public QWidget {
  Q_OBJECT
public:
  PropertyOptionCheckBox(QWidget *parent, QtProperty *property, const QString &optionName)
      : QWidget(parent), m_property(property), m_optionName(optionName), m_checked(property->checkOption(optionName)) {
    setFocusPolicy(Qt::StrongFocus);
  }
  void paintEvent(QPaintEvent * /*unused*/) override {
    QStyleOptionButton opt;
    auto state = isChecked() ? QStyle::State_On : QStyle::State_Off;
    opt.state |= state;
    opt.rect = rect();
    opt.rect.setWidth(opt.rect.height());
    QPainter painter(this);
    QApplication::style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &opt, &painter);
  }
  void mousePressEvent(QMouseEvent *event) override {
    event->accept();
    setChecked(!isChecked());
    m_property->setOption(m_optionName, isChecked());
    update();
    emit optionChanged(m_property, m_optionName, isChecked());
  }
  void setChecked(bool on) { m_checked = on; }
  bool isChecked() const { return m_checked; }
signals:
  void optionChanged(QtProperty * /*_t1*/, const QString & /*_t2*/, bool /*_t3*/);

private:
  QtProperty *m_property;
  QString m_optionName;
  bool m_checked;
};

class QtPropertyEditorView : public QTreeWidget {
  Q_OBJECT
public:
  QtPropertyEditorView(QWidget *parent, bool darkTopLevel);

  void setEditorPrivate(QtTreePropertyBrowserPrivate *editorPrivate) { m_editorPrivate = editorPrivate; }

  QTreeWidgetItem *indexToItem(const QModelIndex &index) const { return itemFromIndex(index); }

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
  QtTreePropertyBrowserPrivate *m_editorPrivate;
  bool m_darkTopLevel;
};

class QtTreePropertyBrowserPrivate {
  QtTreePropertyBrowser *q_ptr;
  Q_DECLARE_PUBLIC(QtTreePropertyBrowser)

public:
  QtTreePropertyBrowserPrivate();
  void init(QWidget *parent, const QStringList &options, bool darkTopLevel);

  void propertyInserted(QtBrowserItem *index, QtBrowserItem *const &afterIndex);
  void propertyRemoved(QtBrowserItem *const &index);
  void propertyChanged(QtBrowserItem *const &index);
  QWidget *createEditor(QtProperty *property, QWidget *parent) const { return q_ptr->createEditor(property, parent); }
  QtProperty *indexToProperty(const QModelIndex &index) const;
  QTreeWidgetItem *indexToItem(const QModelIndex &index) const;
  QtBrowserItem *indexToBrowserItem(const QModelIndex &index) const;
  bool lastColumn(int column) const;
  void disableItem(QTreeWidgetItem *item) const;
  void enableItem(QTreeWidgetItem *item) const;
  bool hasValue(QTreeWidgetItem *const &item) const;
  void hideColumn(int col);
  void showColumn(int col);

  void slotCollapsed(const QModelIndex &index);
  void slotExpanded(const QModelIndex &index);

  QColor calculatedBackgroundColor(QtBrowserItem *item) const;

  QtPropertyEditorView *treeWidget() const { return m_treeWidget; }
  bool markPropertiesWithoutValue() const { return m_markPropertiesWithoutValue; }

  QtBrowserItem *currentItem() const;
  void setCurrentItem(QtBrowserItem *const &browserItem, bool block);
  void editItem(QtBrowserItem *const &browserItem);
  QTreeWidgetItem *getItemWidget(QtBrowserItem *const &browserItem);
  void disableItem(QtBrowserItem *const &item);

  void slotCurrentBrowserItemChanged(QtBrowserItem *item);
  void slotCurrentTreeItemChanged(QTreeWidgetItem *const &newItem, QTreeWidgetItem * /*unused*/);

  QTreeWidgetItem *editedItem() const;
  void closeEditor();

  const QStringList &options() const { return m_options; }
  void setColumnSizes(int s0, int s1, int s2);
  void setStretchLastColumn(bool stretch);

private:
  void updateItem(QTreeWidgetItem *item);

  QMap<QtBrowserItem *, QTreeWidgetItem *> m_indexToItem;
  QMap<QTreeWidgetItem *, QtBrowserItem *> m_itemToIndex;

  QMap<QtBrowserItem *, QColor> m_indexToBackgroundColor;

  QtPropertyEditorView *m_treeWidget;

  bool m_headerVisible;
  QtTreePropertyBrowser::ResizeMode m_resizeMode;
  class QtPropertyEditorDelegate *m_delegate;
  bool m_markPropertiesWithoutValue;
  bool m_browserChangedBlocked;
  QIcon m_expandIcon;
  QStringList m_options; // options that can be associated with QtProperties
};

class QtPropertyEditorDelegate : public QItemDelegate {
  Q_OBJECT
public:
  QtPropertyEditorDelegate(QObject *parent = nullptr)
      : QItemDelegate(parent), m_editorPrivate(nullptr), m_editedItem(nullptr), m_editedWidget(nullptr) {}

  void setEditorPrivate(QtTreePropertyBrowserPrivate *editorPrivate) { m_editorPrivate = editorPrivate; }

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

  void setModelData(QWidget * /*editor*/, QAbstractItemModel * /*model*/,
                    const QModelIndex & /*index*/) const override {}

  void setEditorData(QWidget * /*editor*/, const QModelIndex & /*index*/) const override {}

  bool eventFilter(QObject *object, QEvent *event) override;
  void closeEditor(QtProperty *property);

  QTreeWidgetItem *editedItem() const { return m_editedItem; }

signals:
  void optionChanged(QtProperty * /*_t1*/, const QString & /*_t2*/, bool /*_t3*/);

private slots:
  void slotEditorDestroyed(QObject *object);

private:
  int indentation(const QModelIndex &index) const;

  using EditorToPropertyMap = QMap<const QWidget *, QtProperty *>;
  mutable EditorToPropertyMap m_editorToProperty;

  using PropertyToEditorMap = QMap<QtProperty *, QWidget *>;
  mutable PropertyToEditorMap m_propertyToEditor;
  QtTreePropertyBrowserPrivate *m_editorPrivate;
  mutable QTreeWidgetItem *m_editedItem;
  mutable QWidget *m_editedWidget;
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif
