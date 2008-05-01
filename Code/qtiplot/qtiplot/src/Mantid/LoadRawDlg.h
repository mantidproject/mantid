
#ifndef LOADRAWDLG_H
#define LOADRAWDLG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QString;

class loadRawDlg : public QDialog
{
	Q_OBJECT
	
public:
	loadRawDlg(QWidget *parent = 0);
	~loadRawDlg();

	const QString& getFilename() { return fileName; }
	const QString& getWorkspaceName() { return workspaceName; }

protected:
	
private slots:
	void browseClicked();
	void loadClicked();

private:
	QString fileName;
	QString workspaceName;

	QLabel *label;
	QLabel *label2;

	QLineEdit *lineFile;
	QLineEdit *lineName;

	QPushButton *browseButton;
	QPushButton *loadButton;
	QPushButton *cancelButton;

};

#endif /* LOADRAWDLG_H */
