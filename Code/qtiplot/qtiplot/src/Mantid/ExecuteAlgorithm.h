#ifndef EXECUTEALGORITHM_H
#define EXECUTEALGORITHM_H

#include <QDialog>
#include <vector>
#include <string>

class QLabel;
class QLineEdit;
class QPushButton;
class QString;

#include "MantidPythonAPI/PythonInterface.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Property.h"

class ExecuteAlgorithm : public QDialog
{
	Q_OBJECT
	
public:
	ExecuteAlgorithm(QWidget *parent = 0);
	~ExecuteAlgorithm();
	void CreateLayout(std::vector<Mantid::Kernel::Property*>& properties);
	std::vector<std::string> results;

protected:
	
private slots:
	void okClicked();

private:
	QWidget* m_parent;
	QVector<QLineEdit*> edits;
	QPushButton *okButton;
	QPushButton *exitButton;

	

};

#endif /* EXECUTEALGORITHM_H */
