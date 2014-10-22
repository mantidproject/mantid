#include "MantidQtMantidWidgets/HintingLineEdit.h"

#include <boost/algorithm/string.hpp>
#include <QToolTip>

namespace MantidQt
{
  namespace MantidWidgets
  {

    HintingLineEdit::HintingLineEdit(QWidget *parent, const std::map<std::string,std::string> &hints) : QLineEdit(parent), m_hints(hints)
    {
      connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(updateHint(const QString&)));
    }

    HintingLineEdit::~HintingLineEdit()
    {
    }

    void HintingLineEdit::updateMatches()
    {
      m_matches.clear();

      for(auto it = m_hints.begin(); it != m_hints.end(); ++it)
      {
        const std::string& hint = it->first;

        if(hint.length() < m_curKey.length())
          continue;

        const std::string hintPrefix = hint.substr(0, m_curKey.length());

        if(m_curKey == hintPrefix)
          m_matches[hint] = it->second;
      }
    }

    void HintingLineEdit::updateHint(const QString& text)
    {
      const size_t curPos = (size_t)cursorPosition();
      const std::string line = text.toStdString();

      //Get text from start -> cursor
      std::string prefix = line.substr(0, curPos);

      std::size_t startPos = prefix.find_last_of(",");
      if(startPos != std::string::npos)
        prefix = prefix.substr(startPos + 1, prefix.size() - (startPos + 1));

      boost::trim(prefix);

      m_curKey = prefix;
      showHint();
    }

    void HintingLineEdit::showHint()
    {
      updateMatches();
      //If we're typing the key, auto complete and show matching keys

      //If we're typing the value, show the detailed description for this key
      QString matchList;

      for(auto mIt = m_matches.begin(); mIt != m_matches.end(); ++mIt)
        matchList += QString::fromStdString(mIt->first) + " : " + QString::fromStdString(mIt->second) + "\n";

      QToolTip::showText(mapToGlobal(QPoint(0, 5)), matchList.trimmed());
    }
  } //namespace MantidWidgets
} //namepsace MantidQt
