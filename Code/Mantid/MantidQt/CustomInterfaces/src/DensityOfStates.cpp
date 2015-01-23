#include "MantidQtCustomInterfaces/DensityOfStates.h"

#include <QFileInfo>
#include <QString>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		DensityOfStates::DensityOfStates(QWidget * parent) :
			IndirectSimulationTab(parent)
		{
			m_uiForm.setupUi(parent);
		}

    void DensityOfStates::setup()
    {
    }

    /**
     * Validate the form to check the program can be run.
     *
     * @return Whether the form was valid
     */
		bool DensityOfStates::validate()
		{
      //TODO
      return true;
		}

    /**
     * Configures and executes the LoadSassena algorithm.
     */
		void DensityOfStates::run()
    {
      //TODO
		}

    /**
     * Set the data selectors to use the default save directory
     * when browsing for input files.
     *
     * @param settings :: The settings to loading into the interface
     */
		void DensityOfStates::loadSettings(const QSettings& settings)
		{
			m_uiForm.mwInputFile->readSettings(settings.group());
		}

	} // namespace CustomInterfaces
} // namespace MantidQt
