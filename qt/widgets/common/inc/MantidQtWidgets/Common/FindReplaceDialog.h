#ifndef FINDREPLACEDIALOG_H_
#define FINDREPLACEDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include <QDialog>

//----------------------------------
// Forward declarations
//----------------------------------
class ScriptEditor;
class QPushButton;
class QCheckBox;
class QComboBox;
class QGridLayout;
class QVBoxLayout;
class QShowEvent;

/**
 * Raises a dialog allowing the user to find/replace
 * text in the editor.
 */
class EXPORT_OPT_MANTIDQT_COMMON FindReplaceDialog : public QDialog {
  Q_OBJECT

public:
  /// Constructor
  FindReplaceDialog(ScriptEditor *editor, Qt::WindowFlags fl = nullptr);

protected:
  /// Create the layout
  void initLayout();
  /// Add replace box
  void addReplaceBox();
  /// Add the replace buttons
  void addReplaceButtons();

protected slots:
  /// Find
  bool find(bool backwards = false);
  /// Replace slot
  void replace();
  /// Replace all slot
  void replaceAll();

  /// A slot for the findClicked button
  void findClicked();
  /// Reset the search flags due to changes
  void resetSearchFlags();
  /// Set the flag about whether we are currently finding
  void findNotInProgress();
  /// Clear the editor selection
  void clearEditorSelection();

private:
  /// Called when the widget is shown
  void showEvent(QShowEvent *event) override;

  /// The text editor we are working on
  ScriptEditor *m_editor;
  /// Find next match button
  QPushButton *buttonNext;
  /// Replace text button
  QPushButton *buttonReplace;
  /// Replace all text button
  QPushButton *buttonReplaceAll;
  /// Cancel dialog button
  QPushButton *buttonCancel;

  /// Find box
  QComboBox *boxFind;
  /// Replace box
  QComboBox *boxReplace;

  /// Case-sensitive check box
  QCheckBox *boxCaseSensitive;
  /// Whole words check box
  QCheckBox *boxWholeWords;
  /// Search backwards
  QCheckBox *boxSearchBackwards;
  /// Wrap around
  QCheckBox *boxWrapAround;
  /// Treat as regular expressions
  QCheckBox *boxRegex;

  /// If a find is in progress
  bool m_findInProgress;

  QGridLayout *m_topLayout;
  QVBoxLayout *m_vb2;
};

#endif // FINDREPLACEDIALOG_H_
