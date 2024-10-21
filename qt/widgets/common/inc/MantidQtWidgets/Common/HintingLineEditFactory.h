// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QPainter>
#include <QStyledItemDelegate>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/HintStrategy.h"
#include "MantidQtWidgets/Common/HintingLineEdit.h"
#include <boost/scoped_ptr.hpp>

namespace MantidQt {
namespace MantidWidgets {
/** HintingLineEditFactory : A QStyledItemDelegate that produces
HintingLineEdits using the given hint strategy.
*/
class HintingLineEditFactory : public QStyledItemDelegate {
public:
  HintingLineEditFactory(QAbstractItemDelegate *cellPainterDelegate, std::unique_ptr<HintStrategy> hintStrategy,
                         QObject *parent = nullptr)
      : QStyledItemDelegate(parent), m_strategy(std::move(hintStrategy)), m_cellPainterDelegate(cellPainterDelegate) {};

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto editor = new HintingLineEdit(parent, m_strategy->createHints());
    editor->setFrame(false);

    return editor;
  }

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
    m_cellPainterDelegate->paint(painter, option, index);
  }

protected:
  std::unique_ptr<HintStrategy> m_strategy;
  QAbstractItemDelegate *m_cellPainterDelegate;
};
} // namespace MantidWidgets
} // namespace MantidQt
