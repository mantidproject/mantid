#ifndef MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H
#define MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H

#include <QStyledItemDelegate>
#include <QPainter>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtWidgets/Common/HintingLineEdit.h"
#include "MantidQtWidgets/Common/HintStrategy.h"
#include <boost/scoped_ptr.hpp>

namespace MantidQt {
namespace MantidWidgets {
/** HintingLineEditFactory : A QStyledItemDelegate that produces
HintingLineEdits using the given hint strategy.

Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class HintingLineEditFactory : public QStyledItemDelegate {
public:
  HintingLineEditFactory(HintStrategy *hintStrategy)
      : m_strategy(hintStrategy){};

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override {
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto editor = new HintingLineEdit(parent, m_strategy->createHints());
    editor->setFrame(false);

    return editor;
  }

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {
    QStyledItemDelegate::paint(painter, option, index);

    painter->save();
    painter->setPen(QColor(Qt::black));
    painter->drawRect(option.rect);
    painter->restore();
  }

protected:
  boost::scoped_ptr<HintStrategy> m_strategy;
};
}
}

#endif /* MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H */
