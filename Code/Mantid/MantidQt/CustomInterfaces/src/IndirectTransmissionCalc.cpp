#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidQtCustomInterfaces/IndirectTransmissionCalc.h"

#include <QFileInfo>
#include <QStringList>

using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    IndirectTransmissionCalc::IndirectTransmissionCalc(QWidget * parent) :
      IndirectToolsTab(parent)
    {
      m_uiForm.setupUi(parent);

      connect(m_batchAlgoRunner, SIGNAL(batchCOmplete(bool)), this, SLOT(algorithmComplete(bool)));

      connect(m_uiForm.cbInstrument, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(instrumentSelected(const QString&)));
      connect(m_uiForm.cbAnalyser, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(analyserSelected(const QString&)));
    }

    /*
     * Run any tab setup code.
     */
    void IndirectTransmissionCalc::setup()
    {
    }

    /**
     * Validate the form to check the algorithm can be run.
     *
     * @return Whether the form was valid
     */
    bool IndirectTransmissionCalc::validate()
    {
      // TODO: Validation
      return true;
    }

    /**
     * Run the tab, invoking the IndirectTransmission algorithm.
     */
    void IndirectTransmissionCalc::run()
    {
      // TODO: Run algorithm
    }

    /**
     * Handles completion of the IndirectTransmission algorithm.
     *
     * @param error If the algorithm encountered an error during execution
     */
    void IndirectTransmissionCalc::algorithmComplete(bool error)
    {
      if(error)
        return;

      // TODO: Update table in UI
    }

    /**
     * Set the file browser to use the default save directory
     * when browsing for input files.
     *
     * @param settings The settings to loading into the interface
     */
    void IndirectTransmissionCalc::loadSettings(const QSettings& settings)
    {
      UNUSED_ARG(settings);
    }

    /**
     * Handles an instrument being selected.
     *
     * Populates the analyser and reflection lists.
     *
     * @param instrumentName Name of selected instrument
     */
    void IndirectTransmissionCalc::instrumentSelected(const QString& instrumentName)
    {
      // TODO: Update analyser and reflection list
    }

    /**
     * Handles an analyser being selected.
     *
     * Populates the reflection list.
     *
     * @param analyserName Name of selected analyser
     */
    void IndirectTransmissionCalc::analyserSelected(const QString& analyserName)
    {
      // TODO: Update reflection list
    }

  } // namespace CustomInterfaces
} // namespace MantidQt
