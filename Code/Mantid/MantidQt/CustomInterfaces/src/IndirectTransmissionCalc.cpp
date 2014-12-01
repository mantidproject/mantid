#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidQtCustomInterfaces/IndirectTransmissionCalc.h"

#include <QFileInfo>
#include <QStringList>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    IndirectTransmissionCalc::IndirectTransmissionCalc(QWidget * parent) :
      IndirectToolsTab(parent)
    {
      m_uiForm.setupUi(parent);
    }

    /**
     * Validate the form to check the program can be run
     *
     * @return Whether the form was valid
     */
    bool IndirectTransmissionCalc::validate()
    {
      return true;
    }

    /**
     * Collect the settings on the GUI and build a python
     * script that runs IndirectTransmissionCalc
     */
    void IndirectTransmissionCalc::run()
    {
    }

    /**
     * Set the file browser to use the default save directory
     * when browsing for input files.
     *
     * @param settings The settings to loading into the interface
     */
    void IndirectTransmissionCalc::loadSettings(const QSettings& settings)
    {
    }

  } // namespace CustomInterfaces
} // namespace MantidQt
