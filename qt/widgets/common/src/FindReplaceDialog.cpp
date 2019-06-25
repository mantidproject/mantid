// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FindReplaceDialog.h"
#include "MantidQtWidgets/Common/ScriptEditor.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegExp>
#include <QVBoxLayout>

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Constructor
 */
FindReplaceDialog::FindReplaceDialog(ScriptEditor *editor, Qt::WindowFlags fl)
    : QDialog(editor, fl), m_editor(editor), m_findInProgress(false) {
  initLayout();
  setSizeGripEnabled(true);
}

/**
 * Create the widgets and lay them out
 */
void FindReplaceDialog::initLayout() {
  QGroupBox *gb1 = new QGroupBox();
  m_topLayout = new QGridLayout(gb1);

  m_topLayout->addWidget(new QLabel(tr("Find")), 0, 0);
  boxFind = new QComboBox();
  boxFind->setEditable(true);
  boxFind->setDuplicatesEnabled(false);
  boxFind->setInsertPolicy(QComboBox::InsertAtTop);
  boxFind->setAutoCompletion(true);
  boxFind->setMaxCount(10);
  boxFind->setMaxVisibleItems(10);
  boxFind->setMinimumWidth(250);
  boxFind->setSizePolicy(
      QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  connect(boxFind, SIGNAL(editTextChanged(const QString &)), this,
          SLOT(resetSearchFlags()));

  m_topLayout->addWidget(boxFind, 0, 1);

  addReplaceBox();

  QGroupBox *gb2 = new QGroupBox();
  QGridLayout *bottomLayout = new QGridLayout(gb2);
  QButtonGroup *find_options = new QButtonGroup(this);
  find_options->setExclusive(false);

  boxCaseSensitive = new QCheckBox(tr("&Match case"));
  boxCaseSensitive->setChecked(false);
  bottomLayout->addWidget(boxCaseSensitive, 0, 0);
  find_options->addButton(boxCaseSensitive);

  boxWholeWords = new QCheckBox(tr("&Whole word"));
  boxWholeWords->setChecked(false);
  bottomLayout->addWidget(boxWholeWords, 1, 0);
  find_options->addButton(boxWholeWords);

  boxRegex = new QCheckBox(tr("&Regular expression"));
  boxRegex->setChecked(false);
  bottomLayout->addWidget(boxRegex, 2, 0);
  find_options->addButton(boxRegex);

  boxSearchBackwards = new QCheckBox(tr("&Search backwards"));
  boxSearchBackwards->setChecked(false);
  bottomLayout->addWidget(boxSearchBackwards, 0, 1);
  find_options->addButton(boxSearchBackwards);

  boxWrapAround = new QCheckBox(tr("&Wrap around"));
  boxWrapAround->setChecked(true);
  bottomLayout->addWidget(boxWrapAround, 1, 1);
  find_options->addButton(boxWrapAround);
  connect(find_options, SIGNAL(buttonClicked(int)), this,
          SLOT(resetSearchFlags()));

  QVBoxLayout *vb1 = new QVBoxLayout();
  vb1->addWidget(gb1);
  vb1->addWidget(gb2);

  m_vb2 = new QVBoxLayout();

  buttonNext = new QPushButton(tr("&Next"));
  buttonNext->setShortcut(tr("Ctrl+F"));
  buttonNext->setDefault(true);
  m_vb2->addWidget(buttonNext);
  connect(buttonNext, SIGNAL(clicked()), this, SLOT(findClicked()));

  addReplaceButtons();

  buttonCancel = new QPushButton(tr("&Close"));
  m_vb2->addWidget(buttonCancel);
  m_vb2->addStretch();
  connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));

  QHBoxLayout *hb = new QHBoxLayout(this);
  hb->addLayout(vb1);
  hb->addLayout(m_vb2);
}

/// Add replace box
void FindReplaceDialog::addReplaceBox() {
  setWindowTitle(tr("MantidPlot") + " - " + tr("Find and Replace"));
  m_topLayout->addWidget(new QLabel(tr("Replace with")), 1, 0);
  boxReplace = new QComboBox();
  boxReplace->setEditable(true);
  boxReplace->setDuplicatesEnabled(false);
  boxReplace->setInsertPolicy(QComboBox::InsertAtTop);
  boxReplace->setAutoCompletion(true);
  boxReplace->setMaxCount(10);
  boxReplace->setMaxVisibleItems(10);
  boxReplace->setSizePolicy(
      QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  m_topLayout->addWidget(boxReplace, 1, 1);
  m_topLayout->setColumnStretch(1, 10);
}

/// Add the replace buttons
void FindReplaceDialog::addReplaceButtons() {
  buttonReplace = new QPushButton(tr("&Replace"));
  connect(buttonReplace, SIGNAL(clicked()), this, SLOT(replace()));
  m_vb2->addWidget(buttonReplace);

  buttonReplaceAll = new QPushButton(tr("Replace &all"));
  connect(buttonReplaceAll, SIGNAL(clicked()), this, SLOT(replaceAll()));
  m_vb2->addWidget(buttonReplaceAll);
}

//------------------------------------------------------
// Protected slot member functions
//------------------------------------------------------
/**
 * Find the current search term
 * @param backwards :: If true then the search procedes backwards from the
 * cursor's current position
 * @returns A boolean indicating success/failure
 */
bool FindReplaceDialog::find(bool backwards) {
  QString searchString = boxFind->currentText();
  if (searchString.isEmpty()) {
    QMessageBox::warning(
        this, tr("Empty Search Field"),
        tr("The search field is empty. Please enter some text and try again."));
    boxFind->setFocus();
    return false;
  }

  if (boxFind->findText(searchString) == -1) {
    boxFind->addItem(searchString);
  }

  if (m_findInProgress) {
    m_findInProgress = m_editor->findNext();
  } else {
    bool cs = boxCaseSensitive->isChecked();
    bool whole = boxWholeWords->isChecked();
    bool wrap = boxWrapAround->isChecked();
    bool regex = boxRegex->isChecked();
    m_findInProgress =
        m_editor->findFirst(searchString, regex, cs, whole, wrap, !backwards);
  }
  return m_findInProgress;
}

/**
 * Replace the next occurrence of the search term with the replacement text
 */
void FindReplaceDialog::replace() {
  QString searchString = boxFind->currentText();
  if (searchString.isEmpty()) {
    QMessageBox::warning(
        this, tr("Empty Search Field"),
        tr("The search field is empty. Please enter some text and try again."));
    boxFind->setFocus();
    return;
  }

  if (!m_editor->hasSelectedText() ||
      m_editor->selectedText() != searchString) {
    find(); // find and select next match
    return;
  }

  QString replaceString = boxReplace->currentText();
  m_editor->replace(replaceString);
  find(); // find and select next match

  if (boxReplace->findText(replaceString) == -1) {
    boxReplace->addItem(replaceString);
  }
}

/**
 * Replace all occurrences of the current search term with the replacement text
 */
void FindReplaceDialog::replaceAll() {
  QString searchString = boxFind->currentText();
  if (searchString.isEmpty()) {
    QMessageBox::warning(
        this, tr("Empty Search Field"),
        tr("The search field is empty. Please enter some text and try again."));
    boxFind->setFocus();
    return;
  }

  if (boxFind->findText(searchString) == -1) {
    boxFind->addItem(searchString);
  }

  QString replaceString = boxReplace->currentText();
  if (boxReplace->findText(replaceString) == -1) {
    boxReplace->addItem(replaceString);
  }

  bool regex = boxRegex->isChecked();
  bool cs = boxCaseSensitive->isChecked();
  bool whole = boxWholeWords->isChecked();
  bool wrap = boxWrapAround->isChecked();
  bool backward = boxSearchBackwards->isChecked();
  m_editor->replaceAll(searchString, replaceString, regex, cs, whole, wrap,
                       !backward);
}

/**
 * Find button clicked slot
 */
void FindReplaceDialog::findClicked() {
  // Forward to worker function
  find(boxSearchBackwards->isChecked());
}

/// Reset the search flags due to changes
void FindReplaceDialog::resetSearchFlags() {
  findNotInProgress();
  clearEditorSelection();
}

/**
 * Flip the in-progress flag
 */
void FindReplaceDialog::findNotInProgress() { m_findInProgress = false; }

/**
 * Clear the selection in the editor
 */
void FindReplaceDialog::clearEditorSelection() {
  m_editor->setSelection(-1, -1, -1, -1);
}

/**
 * Called when the widget is shown
 * @param event :: Parameterizes the dialog
 */
void FindReplaceDialog::showEvent(QShowEvent *event) {
  Q_UNUSED(event);
  if (m_editor->hasSelectedText()) {
    QString text = m_editor->selectedText();
    boxFind->setEditText(text);
    boxFind->addItem(text);
  }
}
