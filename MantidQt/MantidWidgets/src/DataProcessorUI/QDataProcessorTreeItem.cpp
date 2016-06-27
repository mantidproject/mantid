/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
    QDataProcessorTreeItem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include <QStringList>

#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTreeItem.h"

QDataProcessorTreeItem::QDataProcessorTreeItem(
    const QVector<QVariant> &data, QDataProcessorTreeItem *parentItem)
    : m_parentItem(parentItem), m_itemData(data) {}

QDataProcessorTreeItem::~QDataProcessorTreeItem() { qDeleteAll(m_childItems); }

QDataProcessorTreeItem *QDataProcessorTreeItem::child(int number) {
  return m_childItems.value(number);
}

int QDataProcessorTreeItem::childCount() const { return m_childItems.count(); }

int QDataProcessorTreeItem::childNumber() const {
  if (m_parentItem)
    return m_parentItem->m_childItems.indexOf(
        const_cast<QDataProcessorTreeItem *>(this));

  return 0;
}

int QDataProcessorTreeItem::columnCount() const { return m_itemData.count(); }

QVariant QDataProcessorTreeItem::data(int column) const {
  return m_itemData.value(column);
}

bool QDataProcessorTreeItem::insertChildren(int position, int count,
                                            int columns) {
  if (position < 0 || position > m_childItems.size())
    return false;

  for (int row = 0; row < count; ++row) {
    QVector<QVariant> data(columns);
    QDataProcessorTreeItem *item = new QDataProcessorTreeItem(data, this);
    // m_childItems.insert(position, item);
    if (m_childItems.count() <= position) {
      m_childItems.insert(position, item);
    } else {
      m_childItems[position] = item;
    }
  }

  return true;
}

bool QDataProcessorTreeItem::insertColumns(int position, int columns) {
  if (position < 0 || position > m_itemData.size())
    return false;

  for (int column = 0; column < columns; ++column)
    m_itemData.insert(position, QVariant());

  foreach (QDataProcessorTreeItem *child, m_childItems)
    child->insertColumns(position, columns);

  return true;
}

QDataProcessorTreeItem *QDataProcessorTreeItem::parent() {
  return m_parentItem;
}

bool QDataProcessorTreeItem::removeChildren(int position, int count) {
  if (position < 0 || position + count > m_childItems.size())
    return false;

  for (int row = 0; row < count; ++row)
    delete m_childItems.takeAt(position);

  return true;
}

bool QDataProcessorTreeItem::removeColumns(int position, int columns) {
  if (position < 0 || position + columns > m_itemData.size())
    return false;

  for (int column = 0; column < columns; ++column)
    m_itemData.remove(position);

  foreach (QDataProcessorTreeItem *child, m_childItems)
    child->removeColumns(position, columns);

  return true;
}

bool QDataProcessorTreeItem::setData(int column, const QVariant &value) {
  if (column < 0 || column >= m_itemData.size())
    return false;

  m_itemData[column] = value;
  return true;
}