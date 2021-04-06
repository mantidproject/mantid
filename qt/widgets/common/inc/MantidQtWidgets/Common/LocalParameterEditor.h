// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QWidget>

class QLineEdit;
class QAction;
class QPushButton;

namespace MantidQt {
namespace MantidWidgets {

class EditLocalParameterDialog;
/**
 * An editor widget for editing a local parameter value.
 * It allows edit the values as well as fix/unfix the parameter.
 */
class LocalParameterEditor : public QWidget {
  Q_OBJECT
public:
  LocalParameterEditor(QWidget *parent, int index, double value, bool fixed, const QString &tie,
                       const QString &constraint, bool othersFixed, bool allOthersFixed, bool othersTied,
                       bool logOptionsEnabled);
signals:
  void setAllValues(double /*_t1*/);
  void fixParameter(int /*_t1*/, bool /*_t2*/);
  void setAllFixed(bool /*_t1*/);
  void setTie(int /*_t1*/, QString /*_t2*/);
  void setTieAll(QString /*_t1*/);
  void setConstraint(int /*_t1*/, QString /*_t2*/);
  void setConstraintAll(QString /*_t1*/);
  void setValueToLog(int /*_t1*/);
  void setAllValuesToLog();
private slots:
  void setAll();
  void fixParameter();
  void fixAll();
  void unfixAll();
  void setTie();
  void removeTie();
  void setTieAll();
  void removeAllTies();
  void setConstraint();
  void removeConstraint();
  void setConstraintAll();
  void removeAllConstraints();
  void updateValue(const QString &value);
  void setToLog();
  void setLogOptionsEnabled(bool enabled);

private:
  bool eventFilter(QObject *widget, QEvent *evn) override;
  void setEditorState();
  static QString setTieDialog(const QString &tie);
  static QString setConstraintDialog(const QString &tie);
  QLineEdit *m_editor;
  QPushButton *m_button;
  QAction *m_setAllAction;
  QAction *m_fixAction;
  QAction *m_fixAllAction;
  QAction *m_unfixAllAction;
  QAction *m_setTieAction;
  QAction *m_removeTieAction;
  QAction *m_setTieToAllAction;
  QAction *m_removeAllTiesAction;
  QAction *m_setConstraintAction;
  QAction *m_removeConstraintAction;
  QAction *m_setConstraintToAllAction;
  QAction *m_removeAllConstraintsAction;
  QAction *m_setToLogAction;
  QAction *m_setAllToLogAction;

  int m_index;
  QString m_value;
  bool m_fixed;
  QString m_tie;
  QString m_constraint;
  bool m_othersFixed;
  bool m_allOthersFixed;
  bool m_othersTied;
  bool m_othersConstrained;
};

} // namespace MantidWidgets
} // namespace MantidQt
