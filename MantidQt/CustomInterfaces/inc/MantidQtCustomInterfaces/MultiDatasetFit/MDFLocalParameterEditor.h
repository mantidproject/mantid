#ifndef MDFLOCALPARAMETEREDITOR_H_
#define MDFLOCALPARAMETEREDITOR_H_

#include <QWidget>

class QLineEdit;
class QAction;
class QPushButton;

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{

/**
 * An editor widget for editing a local parameter value.
 * It allows edit the values as well as fix/unfix the parameter.
 */
class LocalParameterEditor: public QWidget
{
  Q_OBJECT
public:
  LocalParameterEditor(QWidget *parent, int index, double value, bool fixed,
                       QString tie, bool othersFixed, bool allOthersFixed,
                       bool othersTied);
signals:
  void setAllValues(double);
  void fixParameter(int,bool);
  void setAllFixed(bool);
  void setTie(int,QString);
  void setTieAll(QString);
private slots:
  void setAll();
  void fixParameter();
  void fixAll();
  void unfixAll();
  void setTie();
  void removeTie();
  void setTieAll();
  void removeAllTies();
private:
  bool eventFilter(QObject *widget, QEvent *evn);
  void setEditorState();
  static QString setTieDialog(QString tie);
  QLineEdit* m_editor;
  QPushButton* m_button;
  QAction *m_setAllAction;
  QAction *m_fixAction;
  QAction *m_fixAllAction;
  QAction *m_unfixAllAction;
  QAction *m_setTieAction;
  QAction *m_removeTieAction;
  QAction *m_setTieToAllAction;
  QAction *m_removeAllTiesAction;

  int m_index;
  QString m_value;
  bool m_fixed;
  QString m_tie;
  bool m_othersFixed;
  bool m_allOthersFixed;
  bool m_othersTied;
};


} // MDF
} // CustomInterfaces
} // MantidQt


#endif /*MDFDATASETPLOTDATA_H_*/
