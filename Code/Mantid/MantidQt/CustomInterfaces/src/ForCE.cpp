#include "MantidQtCustomInterfaces/ForCE.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		ForCE::ForCE(QWidget * parent) : 
			IndirectForeignTab(parent)
		{
			m_uiForm.setupUi(parent);

		}

		/**
		 * Validate the form to check the program can be run
		 * 
		 * @return :: Whether the form was valid
		 */
		bool ForCE::validate()
		{

			return true;
		}

		/**
		 * Collect the settings on the GUI and build a python
		 * script that runs ForCE
		 */
		void ForCE::run() 
		{
			QString verbose("False");
			QString plot("False");
			QString save("False");

			QString useMap("False");
			QString rejectZero("False");

			QString filename = m_uiForm.mwRun->getFirstFilename();
			QFileInfo finfo(filename);
			QString ext = finfo.extension().toLower();
			QString basename = finfo.basename();

			QSting instrument = m_uiForm.cbInstrument->currentText();
			QString analyser = m_uiForm.cbAnalyser->currentText();
			QString reflection = m_uiForm.cbReflection->currentText();

			if(m_uiForm.chkUseMap->isChecked()){ useMap ="True"; }
			if(m_uiForm.chkRejectZero->isChecked()){ rejectZero ="True"; }

			if(m_uiForm.chkVerbose->isChecked()){ verbose = "True"; }
			if(m_uiForm.chkPlot->isChecked()){ plot = "True"; }
			if(m_uiForm.chkSave->isChecked()){ save ="True"; }

			QString pyFunc ("");
			if(ext == ".asc") //using ascii files
			{
				pyFunc += "IbackStart";
			} 
			else if(ext == ".inx") //using inx files
			{
				pyFunc += "InxStart";
			}

			QString pyInput = 
				"from IndirectForce import "+pyFunc+"\n";

			pyInput += "("+instrument+","+basename+","+analyser+","+reflection+","+rejectZero+","+useMap+""
											","+verbose+","+plot+","+save+")";

			runPythonScript(pyInput);
		}

		/**
		 * Set the data selectors to use the default save directory
		 * when browsing for input files.
		 *  
		 * @param settings :: The settings to loading into the interface
		 */
		void ForCE::loadSettings(const QSettings& settings)
		{
			m_uiForm.mwRun->readSettings(settings.group());
		}
	} // namespace CustomInterfaces
} // namespace MantidQt
