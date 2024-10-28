// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/HintingLineEdit.h"

#include <QKeyEvent>
#include <QLabel>
#include <QStyle>
#include <QToolTip>
#include <boost/algorithm/string.hpp>
#include <utility>

namespace MantidQt::MantidWidgets {
HintingLineEdit::HintingLineEdit(QWidget *parent, std::vector<Hint> hints)
    : QLineEdit(parent), m_hints(std::move(hints)), m_dontComplete(false) {
  m_hintLabel = new QLabel(this, Qt::ToolTip);
  m_hintLabel->setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, m_hintLabel));
  m_hintLabel->setFrameStyle(QFrame::StyledPanel);
  m_hintLabel->setAlignment(Qt::AlignLeft);
  m_hintLabel->setWordWrap(true);
  m_hintLabel->setIndent(1);
  m_hintLabel->setAutoFillBackground(true);
  m_hintLabel->setForegroundRole(QPalette::ToolTipText);
  m_hintLabel->setBackgroundRole(QPalette::ToolTipBase);
  m_hintLabel->ensurePolished();

  connect(this, SIGNAL(textEdited(const QString &)), this, SLOT(updateHints(const QString &)));
  connect(this, SIGNAL(editingFinished()), this, SLOT(hideHints()));
}

HintingLineEdit::~HintingLineEdit() = default;

/** Handle a key press event.

    @param e : A pointer to the event
 */
void HintingLineEdit::keyPressEvent(QKeyEvent *e) {
  m_dontComplete = (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete || e->key() == Qt::Key_Space);

  if (e->key() == Qt::Key_Up) {
    prevSuggestion();
    return;
  }

  if (e->key() == Qt::Key_Down) {
    nextSuggestion();
    return;
  }
  QLineEdit::keyPressEvent(e);
}

/** Rebuild a list of hints whenever the user edits the text, and use the hints
    to make auto completion suggestions.

    @param text : The new contents of the QLineEdit
 */
void HintingLineEdit::updateHints(const QString &text) {
  const size_t curPos = (size_t)cursorPosition();
  const std::string line = text.toStdString();

  // Get everything before the cursor
  std::string prefix = line.substr(0, curPos);

  // Now remove everything before the last ',' to give us the current word
  std::size_t startPos = prefix.find_last_of(",");
  if (startPos != std::string::npos)
    prefix = prefix.substr(startPos + 1, prefix.size() - (startPos + 1));

  // Remove any leading or trailing whitespace
  boost::trim(prefix);

  m_currentPrefix = prefix;

  // Update our current list of matches
  updateMatches();

  // Show the potential matches in a tooltip
  showToolTip();

  // Suggest one of them to the user via auto-completion
  insertSuggestion();
}

/** Hides the list of hints
 */
void HintingLineEdit::hideHints() { m_hintLabel->hide(); }

/** Updates the list of hints matching the user's current input */
void HintingLineEdit::updateMatches() {
  m_matches.clear();
  std::copy_if(m_hints.cbegin(), m_hints.cend(), std::back_inserter(m_matches), [this](Hint const &hint) -> bool {
    auto &hintWord = hint.word();
    return hintWord.length() >= m_currentPrefix.length() &&
           hintWord.substr(0, m_currentPrefix.length()) == m_currentPrefix;
  });
  m_match = m_matches.cbegin();
}

/** Show a tooltip with the current relevant hints */
void HintingLineEdit::showToolTip() {
  QString hintList;
  for (auto const &match : m_matches) {
    hintList += "<b>" + QString::fromStdString(match.word()) + "</b><br />\n";
    if (!match.description().empty())
      hintList += QString::fromStdString(match.description()) + "<br />\n";
  }

  if (!hintList.trimmed().isEmpty()) {
    m_hintLabel->show();
    m_hintLabel->setText(hintList.trimmed());
    m_hintLabel->adjustSize();
    m_hintLabel->move(mapToGlobal(QPoint(0, height())));
  } else {
    m_hintLabel->hide();
  }
}

/** Insert an auto completion suggestion beneath the user's cursor and select it
 */
void HintingLineEdit::insertSuggestion() {
  if (m_currentPrefix.empty() || m_matches.empty() || m_match == m_matches.cend() || m_dontComplete)
    return;

  QString line = text();
  const int curPos = cursorPosition();

  // Don't perform insertions mid-word
  if (curPos + 1 < line.size() && line[curPos + 1].isLetterOrNumber())
    return;

  // Insert a suggestion under the cursor, then select it
  line = line.left(curPos) + QString::fromStdString((*m_match).word()).mid(static_cast<int>(m_currentPrefix.size())) +
         line.mid(curPos);

  setText(line);
  setSelection(curPos, static_cast<int>((*m_match).word().size()));
}

/** Remove any existing auto completion suggestion */
void HintingLineEdit::clearSuggestion() {
  if (!hasSelectedText())
    return;

  // Carefully cut out the selected text
  QString line = text();
  line = line.left(selectionStart()) + line.mid(selectionStart() + selectedText().length());
  setText(line);
}

/** Change to the next available auto completion suggestion */
void HintingLineEdit::nextSuggestion() {
  clearSuggestion();
  // Find the next suggestion in the hint map
  if (m_match != m_matches.end()) {
    ++m_match;
    if (m_match == m_matches.end())
      m_match = m_matches.begin();
    insertSuggestion();
  }
}

/** Change to the previous auto completion suggestion */
void HintingLineEdit::prevSuggestion() {
  clearSuggestion();
  // Find the previous suggestion in the hint map
  if (m_match != m_matches.cend()) {
    if (m_match == m_matches.cbegin()) {
      m_match = m_matches.cend() - 1;
    } else {
      --m_match;
    }
    insertSuggestion();
  }
}
} // namespace MantidQt::MantidWidgets
