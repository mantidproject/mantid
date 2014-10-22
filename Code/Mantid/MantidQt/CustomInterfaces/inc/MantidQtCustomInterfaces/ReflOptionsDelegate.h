#ifndef MANTID_CUSTOMINTERFACES_REFLOPTIONSDELEGATE_H
#define MANTID_CUSTOMINTERFACES_REFLOPTIONSDELEGATE_H

#include <QStyledItemDelegate>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidQtMantidWidgets/HintingLineEdit.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class ReflOptionsDelegate : public QStyledItemDelegate
    {
    public:
      ReflOptionsDelegate() {};
      virtual ~ReflOptionsDelegate() {};
      virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
      {
        Q_UNUSED(option);
        Q_UNUSED(index);

        std::map<std::string,std::string> hints;

        //Create hints
        IAlgorithm_sptr algReflOne = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
        auto properties = algReflOne->getProperties();
        for(auto it = properties.begin(); it != properties.end(); ++it)
          hints[(*it)->name()] = (*it)->briefDocumentation();

        auto editor = new HintingLineEdit(parent, hints);
        editor->setFrame(false);

        return editor;
      }
    };
  }
}

#endif /* MANTID_CUSTOMINTERFACES_REFLOPTIONSDELEGATE_H */
