
#ifndef LOADRAWDLG_H
#define LOADRAWDLG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QString;
class QVBoxLayout;

class loadRawDlg : public QDialog
{
	Q_OBJECT
	
public:
	loadRawDlg(QWidget *parent = 0);
	~loadRawDlg();

	const QString& getFilename() { return fileName; }
	const QString& getWorkspaceName() { return workspaceName; }
	const QString& getSpectrumMin() { return spectrum_min; }
	const QString& getSpectrumMax() { return spectrum_max; }

protected:
	
private slots:
	void browseClicked();
	void loadClicked();

private:
	QString fileName;
	QString workspaceName;
    QString spectrum_min;
    QString spectrum_max;

    QVBoxLayout *mainLayout;

	QLabel *label;
	QLabel *label2;

	QLineEdit *lineFile;
	QLineEdit *lineName;
    QLineEdit *minSpLineEdit;
    QLineEdit *maxSpLineEdit;

	QPushButton *browseButton;
	QPushButton *loadButton;
	QPushButton *cancelButton;

};

#endif /* LOADRAWDLG_H */
