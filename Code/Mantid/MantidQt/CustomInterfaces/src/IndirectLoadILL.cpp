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

      connect(m_uiForm.cbInstrument, SIGNAL(instrumentSelectionChanged(const QString&)), this, SLOT(instrumentChanged(const QString&)));
      connect(m_uiForm.cbAnalyser, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(analyserChanged(const QString&)));
      connect(m_uiForm.mwRun, SIGNAL(filesFound()), this, SLOT(handleFilesFound()));
      connect(m_uiForm.chkUseMap, SIGNAL(toggled(bool)), m_uiForm.mwMapFile, SLOT(setEnabled(bool)));

      QString instrument = m_uiForm.cbInstrument->currentText();
      instrumentChanged(instrument);
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

      QString instrument = m_uiForm.cbInstrument->currentText();
      QString analyser = m_uiForm.cbAnalyser->currentText();
      QString reflection = m_uiForm.cbReflection->currentText();

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
     * Set the analyser option when the instrument changes.
     *
     * @param instrument :: The name of the instrument
     */
    void IndirectLoadILL::instrumentChanged(const QString& instrument)
    {
      using namespace Mantid::API;

      if(!instrument.isEmpty())
      {
        m_uiForm.cbInstrument->blockSignals(true);
        try
        {
          auto inst = getInstrument(instrument);
          if(inst)
          {
            m_paramMap.clear();

            auto analysers = inst->getStringParameter("analysers");

            m_uiForm.cbAnalyser->clear();

            if( analysers.size() > 0 )
            {
              // load analysers and add them to the interface
              QStringList analysersList = QString(analysers[0].c_str()).split(',');
              m_uiForm.cbAnalyser->addItems(analysersList);

              // for each analyser for this instrument, get there reflections
              QStringList::const_iterator it;
              for( it = analysersList.begin(); it != analysersList.end(); ++it) {

                auto reflections = inst->getStringParameter("refl-"+it->toStdString());

                if( reflections.size() > 0 )
                {
                  QStringList refs = QString(reflections[0].c_str()).split(',');

                  // analyser => list of reflections
                  m_paramMap[*it] = refs;
                }
              }

              // set the list of reflections for the current analyser
              analyserChanged(analysersList[0]);
            }
          }
        }
        catch(std::runtime_error& e)
        {
          emit showMessageBox(e.what());
        }
        m_uiForm.cbInstrument->blockSignals(false);
      }
    }

    /**
     * Get a pointer to the instrument.
     *
     * This will use LoadEmptyInstrument and get the instrument details off of
     * the workspace. It also uses ExperimentInfo to get the most relevant instrument
     * defintion.
     *
     * @param instrument :: The name of the instrument
     * @return Pointer to the instrument
     */
    Mantid::Geometry::Instrument_const_sptr IndirectLoadILL::getInstrument(const QString& instrument)
    {
      using namespace Mantid::API;

      MatrixWorkspace_sptr idfWs;

      // Find the file path of the insturment file
      std::string idfPath = ExperimentInfo::getInstrumentFilename(instrument.toStdString());
      if(idfPath.empty())
      {
        throw std::runtime_error("Could not locate instrument file for instrument " + instrument.toStdString());
      }

      // Attempt to load instrument file using LoadEmptyInstrument
      try
      {
        Algorithm_sptr loadEmptyInst = AlgorithmManager::Instance().createUnmanaged("LoadEmptyInstrument", -1);

        loadEmptyInst->initialize();
        loadEmptyInst->setChild(true);
        loadEmptyInst->setRethrows(true);
        loadEmptyInst->setPropertyValue("Filename", idfPath);
        loadEmptyInst->setPropertyValue("OutputWorkspace", "__" + instrument.toStdString() + "_defintion");
        loadEmptyInst->executeAsChildAlg();

        idfWs = loadEmptyInst->getProperty("OutputWorkspace");
      }
      catch(std::runtime_error& ex)
      {
        throw std::runtime_error("Could not load instrument file for instrument" + instrument.toStdString()
              + "\n " + ex.what());
      }

      return idfWs->getInstrument();
    }

    /**
     * Set the reflection option when the analyser changes.
     *
     * @param analyser :: The name of the analyser
     */
    void IndirectLoadILL::analyserChanged(const QString& analyser)
    {
      using namespace Mantid::API;

      if(!analyser.isEmpty())
      {
        m_uiForm.cbReflection->clear();
        m_uiForm.cbReflection->addItems(m_paramMap[analyser]);
      }
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
        //Check if the first part of the name is in the instruments list
        int instrIndex = m_uiForm.cbInstrument->findText(fnameParts[0]);

        if( instrIndex >= 0 )
        {
          m_uiForm.cbInstrument->setCurrentIndex(instrIndex);
        }
      }
    }

  } // namespace CustomInterfaces
} // namespace MantidQt
