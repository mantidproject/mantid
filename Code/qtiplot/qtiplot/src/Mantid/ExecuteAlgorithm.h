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

/**
* This class is used for entering values for properties to use in algorithms.
* It may be inefficient as a number of maps have been added as more functionality
* was added. It may be possible to tidy this up somehow, especially the
* browseClicked method!
**/


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
	void browseClicked();

private:
	std::vector<Mantid::Kernel::Property*> m_props;

	QWidget* m_parent;
	QPushButton *okButton;
	QPushButton *exitButton;

	std::map<QLineEdit*, std::string> edits;
	std::map<QComboBox*, std::string> combos;
	std::map<QPushButton*, QLineEdit*> buttonsToEdits;

};

#endif /* EXECUTEALGORITHM_H */
