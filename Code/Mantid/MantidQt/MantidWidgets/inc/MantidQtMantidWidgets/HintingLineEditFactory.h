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
    class HintingLineEditFactory : public QStyledItemDelegate
    {
    public:
      HintingLineEditFactory(HintStrategy* hintStrategy) : m_strategy(hintStrategy) {};
      virtual ~HintingLineEditFactory()
      {
        delete m_strategy;
      };
      virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
      {
        Q_UNUSED(option);
        Q_UNUSED(index);

        auto editor = new HintingLineEdit(parent, m_strategy->createHints());
        editor->setFrame(false);

        return editor;
      }
    protected:
      HintStrategy* m_strategy;
    };
  }
}

#endif /* MANTID_MANTIDWIDGETS_HINTINGLINEEDITFACTORY_H */
