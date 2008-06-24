#ifndef EXECUTEALGORITHM_H
#define EXECUTEALGORITHM_H

#include <QDialog>
#include <vector>
#include <map>
#include <string>

class QLabel;
class QLineEdit;
class QPushButton;
class QString;
class QComboBox;
class QStringList;

#include "MantidPythonAPI/PythonInterface.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Property.h"

class ExecuteAlgorithm : public QDialog
{
	Q_OBJECT
	
public:
	ExecuteAlgorithm(QWidget *parent = 0);
	~ExecuteAlgorithm();
	void CreateLayout(QStringList& workspaces, std::vector<Mantid::Kernel::Property*>& properties);
	std::map<std::string, std::string> results;

protected:
	
private slots:
	void okClicked();

private:
	QWidget* m_parent;
	QPushButton *okButton;
	QPushButton *exitButton;

	std::map<QLineEdit*, std::string> edits;
	std::map<QComboBox*, std::string> combos;


	

};

#endif /* EXECUTEALGORITHM_H */
