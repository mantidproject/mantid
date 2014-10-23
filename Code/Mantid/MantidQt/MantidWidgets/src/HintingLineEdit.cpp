#include "MantidQtMantidWidgets/HintingLineEdit.h"

#include <boost/algorithm/string.hpp>
#include <QToolTip>

namespace MantidQt
{
  namespace MantidWidgets
  {
    HintingLineEdit::HintingLineEdit(QWidget *parent, const std::map<std::string,std::string> &hints) : QLineEdit(parent), m_hints(hints), m_dontComplete(false)
    {
      connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(updateHints(const QString&)));
    }

    HintingLineEdit::~HintingLineEdit()
    {
    }

    void HintingLineEdit::keyPressEvent(QKeyEvent* e)
    {
      m_dontComplete = (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete || e->key() == Qt::Key_Space);

      if(e->key() == Qt::Key_Up)
      {
        prevSuggestion();
        return;
      }

      if(e->key() == Qt::Key_Down)
      {
        nextSuggestion();
        return;
      }
      QLineEdit::keyPressEvent(e);
    }

    void HintingLineEdit::updateHints(const QString& text)
    {
      const size_t curPos = (size_t)cursorPosition();
      const std::string line = text.toStdString();

      //Get everything before the cursor
      std::string prefix = line.substr(0, curPos);

      //Now remove everything before the last ',' to give us the current word
      std::size_t startPos = prefix.find_last_of(",");
      if(startPos != std::string::npos)
        prefix = prefix.substr(startPos + 1, prefix.size() - (startPos + 1));

      //Remove any leading or trailing whitespace
      boost::trim(prefix);

      //Set the current key/prefix
      m_curKey = prefix;

      //Update our current list of matches
      updateMatches();

      //Show the potential matches in a tooltip
      showToolTip();

      //Suggest one of them to the user via auto-completion
      insertSuggestion();
    }

    void HintingLineEdit::updateMatches()
    {
      m_curMatch.clear();
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

    void HintingLineEdit::showToolTip()
    {
      QString hintList;
      for(auto mIt = m_matches.begin(); mIt != m_matches.end(); ++mIt)
        hintList += QString::fromStdString(mIt->first) + " : " + QString::fromStdString(mIt->second) + "\n";

      QToolTip::showText(mapToGlobal(QPoint(0, 5)), hintList.trimmed());
    }

    void HintingLineEdit::insertSuggestion()
    {
      if(m_curKey.length() < 1 || m_matches.size() < 1 || m_dontComplete)
        return;

      //If we don't have a match, just use the first one in the map
      if(m_curMatch.empty())
        m_curMatch = m_matches.begin()->first;

      QString line = text();
      const int curPos = cursorPosition();

      //Don't perform insertions mid-word
      if(curPos + 1 < line.size() && line[curPos+1].isLetterOrNumber())
        return;

      //Insert a suggestion under the cursor, then select it
      line = line.left(curPos) + QString::fromStdString(m_curMatch).mid((int)m_curKey.size()) + line.mid(curPos);

      setText(line);
      setSelection(curPos, (int)m_curMatch.size());
    }

    void HintingLineEdit::clearSuggestion()
    {
      if(!hasSelectedText())
        return;

      //Carefully cut out the selected text
      QString line = text();
      line = line.left(selectionStart()) + line.mid(selectionStart() + selectedText().length());
      setText(line);
    }

    void HintingLineEdit::nextSuggestion()
    {
      clearSuggestion();
      //Find the next suggestion in the hint map
      auto it = m_matches.find(m_curMatch);
      if(it != m_matches.end())
      {
        it++;
        if(it == m_matches.end())
          m_curMatch = m_matches.begin()->first;
        else
          m_curMatch = it->first;
        insertSuggestion();
      }
    }

    void HintingLineEdit::prevSuggestion()
    {
      clearSuggestion();
      //Find the previous suggestion in the hint map
      auto it = m_matches.find(m_curMatch);
      if(it != m_matches.end())
      {
        it--;
        if(it == m_matches.end())
          m_curMatch = m_matches.rbegin()->first;
        else
          m_curMatch = it->first;
        insertSuggestion();
      }
    }
  } //namespace MantidWidgets
} //namepsace MantidQt
