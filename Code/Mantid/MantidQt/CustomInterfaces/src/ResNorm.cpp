#include "MantidQtCustomInterfaces/ResNorm.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		ResNorm::ResNorm(QWidget * parent) : 
			IndirectBayesTab(parent)
		{
			m_uiForm.setupUi(parent);

			connect(m_uiForm.dsVanadium, SIGNAL(dataReady(const QString&)), this, SLOT(handleVanadiumInputReady(const QString&)));

			//add the properties browser to the ui form
			m_uiForm.treeSpace->addWidget(m_propTree);
			m_properties["EMin"] = m_dblManager->addProperty("EMin");
			m_properties["EMax"] = m_dblManager->addProperty("EMax");
			m_properties["VanBinning"] = m_intManager->addProperty("Van Binning");
			
			m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);

			m_propTree->addProperty(m_properties["EMin"]);
			m_propTree->addProperty(m_properties["EMax"]);
			m_propTree->addProperty(m_properties["VanBinning"]);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
			m_plot->setCanvasBackground(Qt::white);
   		m_plot->setAxisFont(QwtPlot::xBottom, parent->font());
    	m_plot->setAxisFont(QwtPlot::yLeft, parent->font());
		}

		bool ResNorm::validate()
		{
			//check we have files/workspaces available to run with
			if(m_uiForm.dsResolution->getCurrentDataName().isEmpty() ||
					m_uiForm.dsVanadium->getCurrentDataName().isEmpty())
			{
				return false;
			}

			return true;
		}

		void ResNorm::run() 
		{
			QString verbose("False");
			QString plot("False");
			QString save("False");

			QString pyInput = 
				"from IndirectBayes import ResNormRun\n";

			QString VanName = m_uiForm.dsVanadium->getCurrentDataName();
			QString ResName = m_uiForm.dsResolution->getCurrentDataName();

			QString EMin = m_properties["EMin"]->valueText();
			QString EMax = m_properties["EMax"]->valueText();

			QString ERange = "[" + EMin + "," + EMax + "]";

			QString nBin = m_properties["VanBinning"]->valueText();

			if(m_uiForm.ckVerbose->isChecked()){ verbose = "True"; }
			if(m_uiForm.ckPlot->isChecked()){ plot = "True"; }
			if(m_uiForm.ckSave->isChecked()){ save ="True"; }

			pyInput += "ResNormRun("+VanName+", "+ResName+", "+ERange+", "+nBin+","
										" Save="+save+", Plot="+plot+", Verbose="+verbose+")\n";

			runPythonScript(pyInput);
		}

		void ResNorm::handleVanadiumInputReady(const QString& filename)
		{
			plotMiniPlot(filename, 0);
		}
	} // namespace CustomInterfaces
} // namespace MantidQt
