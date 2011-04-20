
#ifndef LOADDAEDLG_H
#define LOADDAEDLG_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QString;
class QVBoxLayout;
class QCheckBox;

class loadDAEDlg : public QDialog
{
	Q_OBJECT
	
public:
	loadDAEDlg(QWidget *parent = 0);
	~loadDAEDlg();

	const QString& getHostName() { return m_hostName; }
	const QString& getWorkspaceName() { return m_workspaceName; }
	const QString& getSpectrumMin() { return m_spectrum_min; }
	const QString& getSpectrumMax() { return m_spectrum_max; }
	const QString& getSpectrumList() { return m_spectrum_list; }
  int updateInterval(){return m_updateInterval;}

protected:
	
private slots:
	void load();
    void changeUpdateState(int);
    void updateIntervalEntered(const QString & text );
  void helpClicked();

private:
	QString m_hostName;
	QString m_workspaceName;
    QString m_spectrum_min;
    QString m_spectrum_max;
    QString m_spectrum_list;
    int m_updateInterval;

	QLineEdit *lineHost;
	QLineEdit *lineName;
    QLineEdit *minSpLineEdit;
    QLineEdit *maxSpLineEdit;
    QLineEdit *listSpLineEdit;
    QCheckBox *updateCheck;
    QLineEdit *updateLineEdit;

};

#endif /* LOADDAEDLG_H */
