
#ifndef LOADRAWDLG_H
#define LOADRAWDLG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QString;
class QVBoxLayout;
class QComboBox;

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
	const QString& getSpectrumList() { return spectrum_list; }
	const QString& getCacheOption() { return cache_option; }

protected:
	
private slots:
	void browseClicked();
	void loadClicked();

private:
	QString fileName;
	QString workspaceName;
    QString spectrum_min;
    QString spectrum_max;
    QString spectrum_list;
    QString cache_option;

    QVBoxLayout *mainLayout;

	QLabel *label;
	QLabel *label2;

	QLineEdit *lineFile;
	QLineEdit *lineName;
    QLineEdit *minSpLineEdit;
    QLineEdit *maxSpLineEdit;
    QLineEdit *listSpLineEdit;
    QComboBox *cacheCBox;

	QPushButton *browseButton;
	QPushButton *loadButton;
	QPushButton *cancelButton;

    QString directory;

};

#endif /* LOADRAWDLG_H */
