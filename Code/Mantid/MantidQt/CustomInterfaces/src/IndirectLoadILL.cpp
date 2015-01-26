#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidQtCustomInterfaces/IndirectLoadILL.h"

#include <QFileInfo>
#include <QStringList>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    IndirectLoadILL::IndirectLoadILL(QWidget * parent) :
      IndirectToolsTab(parent)
    {
      m_uiForm.setupUi(parent);

      connect(m_uiForm.mwRun, SIGNAL(filesFound()), this, SLOT(handleFilesFound()));
      connect(m_uiForm.chkUseMap, SIGNAL(toggled(bool)), m_uiForm.mwMapFile, SLOT(setEnabled(bool)));
    }

    /**
     * Run any tab setup code.
     */
    void IndirectLoadILL::setup()
    {
    }

    /**
     * Validate the form to check the program can be run
     *
     * @return :: Whether the form was valid
     */
    bool IndirectLoadILL::validate()
    {
      QString filename = m_uiForm.mwRun->getFirstFilename();
      QFileInfo finfo(filename);
      QString ext = finfo.extension().toLower();

      if(ext != "asc" && ext != "inx" && ext != "nxs")
      {
        emit showMessageBox("File is not of expected type:\n File type must be .asc, .inx or .nxs");
        return false;
      }

      return true;
    }

    /**
     * Collect the settings on the GUI and build a python
     * script that runs IndirectLoadILL
     */
    void IndirectLoadILL::run()
    {
      QString verbose("False");
      QString plot("False");
      QString save("None");

      QString useMap("False");
      QString rejectZero("False");

      QString filename = m_uiForm.mwRun->getFirstFilename();
      QFileInfo finfo(filename);
      QString ext = finfo.extension().toLower();

      QString instrument = m_uiForm.iicInstrumentConfiguration->getInstrumentName();
      QString analyser = m_uiForm.iicInstrumentConfiguration->getAnalyserName();
      QString reflection = m_uiForm.iicInstrumentConfiguration->getReflectionName();

      if(m_uiForm.chkUseMap->isChecked()){ useMap ="True"; }
      QString mapPath = m_uiForm.mwMapFile->getFirstFilename();

      if(m_uiForm.chkRejectZero->isChecked()){ rejectZero ="True"; }

      //output options
      if(m_uiForm.chkVerbose->isChecked()){ verbose = "True"; }
      if(m_uiForm.chkSave->isChecked()){ save = "True"; }
      plot = m_uiForm.cbPlot->currentText();

      QString pyInput("");
      if(instrument == "IN16B")
      {
        pyInput += "from IndirectCommon import getWSprefix\n";
        pyInput += "tmp_name = '__tmp_IndirectLoadASCII_IN16B'\n";
        pyInput += "LoadILLIndirect(Filename='"+filename+"', OutputWorkspace=tmp_name)\n";
        pyInput += "output_name = getWSprefix(tmp_name) + 'red'\n";
        pyInput += "RenameWorkspace('__tmp_IndirectLoadASCII_IN16B', OutputWorkspace=output_name)\n";
      }
      else
      {
        QString pyFunc ("");
        //IN13 has a different loading routine
        if(instrument == "IN13")
        {
          ext = "asc";
          pyFunc = "IN13Start";
        }
        else if(ext == "asc") //using ascii files
        {
          pyFunc += "IbackStart";
        }
        else if(ext == "inx") //using inx files
        {
          pyFunc += "InxStart";
        }
        else
        {
          emit showMessageBox("Could not find appropriate loading routine for " + filename);
          return;
        }

        pyInput += "from IndirectNeutron import "+pyFunc+"\n";
        pyInput += pyFunc + "('"+instrument+"','"+filename+"','"+analyser+"','"+reflection+"',"+rejectZero+","+useMap+",'"+mapPath+"'"
                        ","+verbose+",'"+plot+"',"+save+")";

      }
      runPythonScript(pyInput);
    }

    /**
     * Set the file browser to use the default save directory
     * when browsing for input files.
     *
     * @param settings :: The settings to loading into the interface
     */
    void IndirectLoadILL::loadSettings(const QSettings& settings)
    {
      m_uiForm.mwRun->readSettings(settings.group());
    }

    /**
     * Set the instrument selected in the combobox based on
     * the file name of the run is possible.
     *
     * Assumes that names have the form \<instrument\>_\<run-number\>.\<ext\>
     */
    void IndirectLoadILL::handleFilesFound()
    {
      //get first part of basename
      QString filename = m_uiForm.mwRun->getFirstFilename();
      QFileInfo finfo(filename);
      QStringList fnameParts = finfo.baseName().split('_');

      if( fnameParts.size() > 0 )
      {
        // Check if the first part of the name is in the instruments list
        m_uiForm.iicInstrumentConfiguration->setInstrument(fnameParts[0]);
      }
    }

  } // namespace CustomInterfaces
} // namespace MantidQt
