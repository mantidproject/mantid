#ifndef MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H
#define MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H

#include <QStyledItemDelegate>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtMantidWidgets/HintingLineEdit.h"
#include "MantidQtMantidWidgets/HintStrategy.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /** HintingLineEditFactory : A QStyledItemDelegate that produces HintingLineEdits using the given hint strategy.

    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class HintingLineEditFactory : public QStyledItemDelegate
    {
    public:
      HintingLineEditFactory(HintStrategy* hintStrategy) : m_strategy(hintStrategy) {};
      virtual ~HintingLineEditFactory() {};
      virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
      {
        Q_UNUSED(option);
        Q_UNUSED(index);

        auto editor = new HintingLineEdit(parent, m_strategy->createHints());
        editor->setFrame(false);

        return editor;
      }
    protected:
      boost::scoped_ptr<HintStrategy> m_strategy;
    };
  }
}

#endif /* MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H */
