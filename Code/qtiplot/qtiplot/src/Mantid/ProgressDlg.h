
#ifndef PROGRESSDLG_H
#define PROGRESSDLG_H

#include "MantidAPI/AlgorithmObserver.h"
#include <string>
#include <QDialog>

class QProgressBar;
class QLabel;

class ProgressDlg : public QDialog, public Mantid::API::AlgorithmObserver
{
	Q_OBJECT
	
public:
	ProgressDlg(Mantid::API::IAlgorithm_sptr alg,QWidget *parent = 0);
	//~ProgressDlg();
    void setValue(int p,const QString& msg);

private slots:
	void cancelClicked();
	void backgroundClicked();

signals:
    void canceled();
    void toBackground();
private:

    /// AlgorithmObserver handlers implementations
    void progressHandle(const Mantid::API::IAlgorithm* alg,double p,const std::string& msg);
    void finishHandle(const Mantid::API::IAlgorithm* alg);
    void errorHandle(const Mantid::API::IAlgorithm* alg,const std::string& what);

    Mantid::API::IAlgorithm_sptr m_alg; ///< Pointer to the running algorithm
    QProgressBar *m_progressBar;
    QLabel *m_message;
};

#endif /* PROGRESSDLG_H */
