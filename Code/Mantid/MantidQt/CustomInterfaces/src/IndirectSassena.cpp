#include "MantidQtCustomInterfaces/IndirectSassena.h"

#include <QFileInfo>
#include <QString>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		IndirectSassena::IndirectSassena(QWidget * parent) :
			IndirectSimulationTab(parent)
		{
			m_uiForm.setupUi(parent);
		}

    /**
     * Validate the form to check the program can be run.
     *
     * @return :: Whether the form was valid
     */
		bool IndirectSassena::validate()
		{
      //TODO

			return true;
		}

    /**
     * TODO
     */
		void IndirectSassena::run()
		{
		}

    /**
     * Set the data selectors to use the default save directory
     * when browsing for input files.
     *
     * @param settings :: The settings to loading into the interface
     */
		void IndirectSassena::loadSettings(const QSettings& settings)
		{
			m_uiForm.mwInputFile->readSettings(settings.group());
		}

	} // namespace CustomInterfaces
} // namespace MantidQt
