#include "MantidQtMantidWidgets/HintingLineEdit.h"

#include <QToolTip>

namespace MantidQt
{
  namespace MantidWidgets
  {

    HintingLineEdit::HintingLineEdit(QWidget *parent, const std::map<std::string,std::string> &hints) : QLineEdit(parent), m_hints(hints)
    {
      connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(showHint(const QString&)));
    }

    HintingLineEdit::~HintingLineEdit()
    {
    }

    void HintingLineEdit::showHint(const QString& keyword)
    {
      //Get the current word from the cursor position

      //If we're typing the key, auto complete and show matching keys

      //If we're typing the value, show the detailed description for this key
      QToolTip::showText(mapToGlobal(QPoint(0, 5)), "You have entered:\n" + keyword);
    }
  } //namespace MantidWidgets
} //namepsace MantidQt
