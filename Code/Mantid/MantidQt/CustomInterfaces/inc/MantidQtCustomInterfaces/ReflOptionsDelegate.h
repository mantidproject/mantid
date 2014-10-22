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

        //Dynamically produce a list of hints and their descriptions from the algorithm
        IAlgorithm_sptr algReflOne = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
        auto properties = algReflOne->getProperties();
        for(auto it = properties.begin(); it != properties.end(); ++it)
        {
          const std::string name = (*it)->name();

          //Blacklist some properties from being suggested
          //These are either useless to the user (such as ThetaOut), or are handled by the presenter
          if(name == "ThetaIn" ||
              name == "ThetaOut" ||
              name == "InputWorkspace" ||
              name == "OutputWorkspace" ||
              name == "OutputWorkspaceWavelength" ||
              name == "FirstTransmissionRun" ||
              name == "SecondTransmissionRun")
            continue;

          hints[name] = (*it)->briefDocumentation();
        }

        auto editor = new HintingLineEdit(parent, hints);
        editor->setFrame(false);

        return editor;
      }
    };
  }
}

#endif /* MANTID_CUSTOMINTERFACES_REFLOPTIONSDELEGATE_H */
