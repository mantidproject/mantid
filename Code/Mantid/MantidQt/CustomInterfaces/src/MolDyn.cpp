#include "MantidQtCustomInterfaces/MolDyn.h"

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

			QString pyInput = 
				"from IndirectBayes import MolDynRun\n";

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
