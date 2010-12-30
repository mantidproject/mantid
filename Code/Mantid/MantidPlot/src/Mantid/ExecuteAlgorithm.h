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

#include "MantidAPI/Workspace.h"
#include "MantidKernel/Property.h"
#include "MantidAPI/Algorithm.h"

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
  ExecuteAlgorithm(QWidget *parent = 0, bool forScript = false);
	~ExecuteAlgorithm();
  void CreateLayout(Mantid::API::IAlgorithm_sptr alg, const QString & message = "");

protected:
	
private slots:
	void okClicked();
	void browseClicked();
	void textChanged();

private:
	Mantid::API::IAlgorithm_sptr m_alg;
	std::vector<Mantid::Kernel::Property*> m_props;

	QWidget* m_parent;
	QPushButton *okButton;
	QPushButton *exitButton;

	std::map<QLineEdit*, std::string> edits;
	std::map<QComboBox*, std::string> combos;
	std::map<QPushButton*, QLineEdit*> buttonsToEdits;
	std::map<std::string, QLabel*> validators;
  
  bool m_forScript;

	bool execute();
    bool setPropertiesAndValidate();
    bool validateProperties();
    bool setPropertyValue(const std::string& name, const std::string& value);
	bool validateProperty(const std::string& name);
	void showValidator(const std::string& propName);
	void hideValidator(const std::string& propName);

    QString m_directory;
};

#endif /* EXECUTEALGORITHM_H */
