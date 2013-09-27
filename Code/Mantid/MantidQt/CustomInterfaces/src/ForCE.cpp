#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtCustomInterfaces/ForCE.h"

#include <QFileInfo>
#include <QStringList>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		ForCE::ForCE(QWidget * parent) : 
			IndirectForeignTab(parent)
		{
			m_uiForm.setupUi(parent);

			connect(m_uiForm.cbInstrument, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(instrumentChanged(const QString&)));
			connect(m_uiForm.cbAnalyser, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(analyserChanged(const QString&)));

			// Setup analysers and reflections
			QString currentIntrument = m_uiForm.cbInstrument->currentText();
			instrumentChanged(currentIntrument);
			QString currentAnalyser = m_uiForm.cbAnalyser->currentText();
			analyserChanged(currentAnalyser);
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
			QString basename = finfo.baseName();

			QString instrument = m_uiForm.cbInstrument->currentText();
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

		/**
		 * Set the analyser and reflection options when the instrument changes.
		 *  
		 * @param settings :: The settings to loading into the interface
		 */
		void ForCE::instrumentChanged(const QString& instrument)
		{
			using namespace Mantid::API;

	    auto inst = getInstrument(instrument);
	    if(inst)
	    {
	    	auto analysers = inst->getStringParameter("analysers");

		    m_uiForm.cbAnalyser->clear();

		    if( analysers.size() > 0 )
		    {
	    		QStringList refs = QString(analysers[0].c_str()).split(',');
	    		m_uiForm.cbAnalyser->addItems(refs);
		    }
	    }
    }

    Mantid::Geometry::Instrument_const_sptr ForCE::getInstrument(const QString& instrument)
    {
    	using namespace Mantid::API;

    	std::string idfPath = ExperimentInfo::getInstrumentFilename(instrument.toStdString());
    	Algorithm_sptr loadEmptyInst = AlgorithmManager::Instance().createUnmanaged("LoadEmptyInstrument", -1);

      loadEmptyInst->initialize();
      loadEmptyInst->setChild(true);
      loadEmptyInst->setRethrows(true);
      loadEmptyInst->setPropertyValue("Filename", idfPath);
      loadEmptyInst->setPropertyValue("OutputWorkspace", "__" + instrument.toStdString() + "_defintion");
      loadEmptyInst->executeAsChildAlg();

			MatrixWorkspace_sptr idfWs = loadEmptyInst->getProperty("OutputWorkspace");
	    return idfWs->getInstrument();
    }

    void ForCE::analyserChanged(const QString& analyser)
    {
    	using namespace Mantid::API;

    	auto inst = getInstrument(m_uiForm.cbInstrument->currentText());

    	m_uiForm.cbReflection->clear();

    	if(inst)
    	{
	    	auto reflections = inst->getStringParameter("refl-"+analyser.toStdString());
	    	
	    	if( reflections.size() > 0 )
	    	{
	    		QStringList refs = QString(reflections[0].c_str()).split(',');
	    		m_uiForm.cbReflection->addItems(refs);
	    	}
    	}
    }

	} // namespace CustomInterfaces
} // namespace MantidQt
