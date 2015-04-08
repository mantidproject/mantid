#ifndef MDFLOCALPARAMETEREDITOR_H_
#define MDFLOCALPARAMETEREDITOR_H_

#include <QWidget>

class QLineEdit;
class QAction;

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
  LocalParameterEditor(QWidget *parent, int index, bool fixed);
signals:
  void setAllValues(double);
  void fixParameter(int,bool);
  void setAllFixed(bool);
private slots:
  void setAll();
  void fixParameter();
  void fixAll();
  void unfixAll();
private:
  QLineEdit* m_editor;
  QAction *m_fixAction;
  int m_index;
  bool m_fixed;
};


} // MDF
} // CustomInterfaces
} // MantidQt


#endif /*MDFDATASETPLOTDATA_H_*/
