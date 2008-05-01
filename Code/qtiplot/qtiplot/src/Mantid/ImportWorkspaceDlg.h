
#ifndef IMPORTWORKSPACEDLG_H
#define IMPORTWORKSPACEDLG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QString;

class ImportWorkspaceDlg : public QDialog
{
	Q_OBJECT
	
public:
	ImportWorkspaceDlg(QWidget *parent = 0, int num = 0);
	~ImportWorkspaceDlg();

	int getLowerLimit() { return lowerLimit; }
	int getUpperLimit() { return upperLimit; }

protected:
	
private slots:
	void okClicked();

private:
	int numHists;
	int lowerLimit;
	int upperLimit;

	QLabel *label;
	QLabel *labelLow;
	QLabel *labelHigh;

	QLineEdit *lineLow;
	QLineEdit *lineHigh;

	QPushButton *okButton;
	QPushButton *cancelButton;

};

#endif /* IMPORTWORKSPACEDLG_H */
