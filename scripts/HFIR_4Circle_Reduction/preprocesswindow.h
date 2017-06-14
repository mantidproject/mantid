#ifndef PREPROCESSWINDOW_H
#define PREPROCESSWINDOW_H

#include <QMainWindow>

namespace Ui {
  class PreprocessWindow;
}

class PreprocessWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit PreprocessWindow(QWidget *parent = 0);
  ~PreprocessWindow();

private:
  Ui::PreprocessWindow *ui;
};

#endif // PREPROCESSWINDOW_H
