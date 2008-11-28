
#ifndef PROGRESSDLG_H
#define PROGRESSDLG_H

#include <QDialog>

class QProgressBar;
class QLabel;

class ProgressDlg : public QDialog
{
	Q_OBJECT
	
public:
	ProgressDlg(QWidget *parent = 0);
	//~ProgressDlg();
    void setValue(int p,const QString& msg);

private slots:
	void cancelClicked();
	void backgroundClicked();

signals:
    void canceled();
    void toBackground();
private:
    QProgressBar *m_progressBar;
    QLabel *m_message;
};

#endif /* PROGRESSDLG_H */
