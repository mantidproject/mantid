#ifndef MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H
#define MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H

#include "MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QtCommandAdapter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtWidgets/Common/HintingLineEditFactory.h"

#include <QClipboard>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QWhatsThis>
#include <QWidget>
#include <QStyledItemDelegate> 
#include <QPainter>

#include <QStyledItemDelegate>
#include <QPainter>
#include <QWidget>

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
  HintingLineEditFactory(HintStrategy *hintStrategy, boost::shared_ptr<MantidQt::MantidWidgets::DataProcessor::AbstractTreeModel> model = 0)
      : m_strategy(hintStrategy), m_model(model){};
  
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override {
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto editor = new HintingLineEdit(parent, m_strategy->createHints());
    editor->setFrame(false);

    return editor;
  }

   void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
   {
       painter->save();
       painter->setPen(QColor(Qt::black));
       if (m_model->isProcessed(index.row(), index.parent())){
          painter->fillRect(option.rect, Qt::green);
       }
       painter->drawRect(option.rect);
       painter->restore();

       QStyledItemDelegate::paint(painter, option, index);
   }

protected:
  boost::scoped_ptr<HintStrategy> m_strategy;
  boost::shared_ptr<MantidQt::MantidWidgets::DataProcessor::AbstractTreeModel> m_model;
};
}
}

#endif /* MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H */
