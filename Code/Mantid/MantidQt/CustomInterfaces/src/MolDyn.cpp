#include "MantidQtCustomInterfaces/MolDyn.h"

#include <QFileInfo>
#include <QString>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		MolDyn::MolDyn(QWidget * parent) : 
			IndirectForeignTab(parent)
		{
			m_uiForm.setupUi(parent);
		}

		/**
		 * Validate the form to check the program can be run
		 * 
		 * @return :: Whether the form was valid
		 */
		bool MolDyn::validate()
		{
			QString filename = m_uiForm.mwRun->getFirstFilename();
			QFileInfo finfo(filename);
			QString ext = finfo.extension().toLower();

			if(ext != "dat" && ext != "cdl")
			{
				emit showMessageBox("File is not of expected type:\n File type must be .dat or .cdl");
				return false;
			} 

			return true;
		}

		/**
		 * Collect the settings on the GUI and build a python
		 * script that runs MolDyn
		 */
		void MolDyn::run() 
		{
			QString verbose("False");
			QString plot("False");
			QString save("False");

			QString filename = m_uiForm.mwRun->getFirstFilename();
			QFileInfo finfo(filename);
			QString ext = finfo.extension().toLower();

			QString funcNames = m_uiForm.leFunctionNames->text();

			//output options
			if(m_uiForm.chkVerbose->isChecked()){ verbose = "True"; }
			if(m_uiForm.chkSave->isChecked()){ save ="True"; }
			plot = m_uiForm.cbPlot->currentText();


			QString pyInput = 
				"from MolDynTransfer import ";

			QString pyFunc("");
			if(ext == "dat")
			{
				pyFunc = "MolDynText";
				pyInput += pyFunc + "\n" + pyFunc + "('"+filename+"',"+verbose+",'"+plot+"',"+save+")";
			}
			else if (ext == "cdl")
			{
				pyFunc = "MolDynImport";
				pyInput += pyFunc + "\n" + pyFunc + "('"+filename+"','"+funcNames+"',"+verbose+",'"+plot+"',"+save+")";
			}

			runPythonScript(pyInput);
		}

		/**
		 * Set the data selectors to use the default save directory
		 * when browsing for input files.
		 *  
		 * @param settings :: The settings to loading into the interface
		 */
		void MolDyn::loadSettings(const QSettings& settings)
		{
			m_uiForm.mwRun->readSettings(settings.group());
		}
	} // namespace CustomInterfaces
} // namespace MantidQt
