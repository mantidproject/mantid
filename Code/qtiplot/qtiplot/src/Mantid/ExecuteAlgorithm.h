#ifndef EXECUTEALGORITHM_H
#define EXECUTEALGORITHM_H

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;
class QString;

#include "MantidPythonAPI/PythonInterface.h"
#include "MantidAPI/Workspace.h"

class ExecuteAlgorithm : public QDialog
{
	Q_OBJECT
	
public:
	ExecuteAlgorithm(QWidget *parent = 0);
	~ExecuteAlgorithm();
	void PassPythonInterface(Mantid::PythonAPI::PythonInterface*);

protected:
	
private slots:

private:
	Mantid::PythonAPI::PythonInterface* interface;

	QWidget* m_parent;

	QLabel *label;
	QPushButton *exitButton;

};

#endif /* EXECUTEALGORITHM_H */
