
#ifndef IMPORTWORKSPACEDLG_H
#define IMPORTWORKSPACEDLG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QString;

class ImportWorkspaceDlg : public QDialog
{
  Q_OBJECT

public:
  ImportWorkspaceDlg(QWidget *parent = 0, size_t num = 0);
  ~ImportWorkspaceDlg();

  int getLowerLimit() { return lowerLimit; }
  int getUpperLimit() { return upperLimit; }
  bool isFiltered(){ return filtered; }
  double getMinValue(){ return minValue; }
  double getMaxValue(){ return maxValue; }

protected:

private slots:
  void okClicked();
  void enableFilter(int state);

private:
  size_t numHists;
  int lowerLimit;
  int upperLimit;
  bool filtered;
  double minValue;
  double maxValue;

  QLabel *label;
  QLabel *labelLow;
  QLabel *labelHigh;

  QLineEdit *lineLow;
  QLineEdit *lineHigh;

  QCheckBox *checkFilter;
  //QLabel *labelFilterMaximum;
  QLineEdit *lineMinimum;
  QLineEdit *lineMaximum;

  QPushButton *okButton;
  QPushButton *cancelButton;

};

#endif /* IMPORTWORKSPACEDLG_H */
