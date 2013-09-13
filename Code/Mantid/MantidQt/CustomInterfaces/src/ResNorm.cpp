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
			m_properties["VanBinning"] = m_dblManager->addProperty("Van Binning");
			
			m_dblManager->setDecimals(m_properties["EMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["EMax"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["VanBinning"], INT_DECIMALS);

			m_propTree->addProperty(m_properties["EMin"]);
			m_propTree->addProperty(m_properties["EMax"]);
			m_propTree->addProperty(m_properties["VanBinning"]);

			//set default values
			m_dblManager->setValue(m_properties["VanBinning"], 1);
			m_dblManager->setMinimum(m_properties["VanBinning"], 1);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
			m_plot->setCanvasBackground(Qt::white);
   		m_plot->setAxisFont(QwtPlot::xBottom, parent->font());
    	m_plot->setAxisFont(QwtPlot::yLeft, parent->font());
		}

		/**
		 * Validate the form to check we can run the program
		 */
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

		/**
		 * Collect the settings on the GUI and build a python
		 * script that runs ResNorm
		 */
		void ResNorm::run() 
		{
			QString verbose("False");
			QString plot("False");
			QString save("False");

			QString pyInput = 
				"from IndirectBayes import ResNormRun\n";

			// get the file names
			QString VanName = m_uiForm.dsVanadium->getCurrentDataName();
			QString ResName = m_uiForm.dsResolution->getCurrentDataName();

			//get the parameters for ResNomr
			QString EMin = m_properties["EMin"]->valueText();
			QString EMax = m_properties["EMax"]->valueText();

			QString ERange = "[" + EMin + "," + EMax + "]";

			QString nBin = m_properties["VanBinning"]->valueText();

			//get output options
			if(m_uiForm.ckVerbose->isChecked()){ verbose = "True"; }
			if(m_uiForm.ckPlot->isChecked()){ plot = "True"; }
			if(m_uiForm.ckSave->isChecked()){ save ="True"; }

			pyInput += "ResNormRun("+VanName+", "+ResName+", "+ERange+", "+nBin+","
										" Save="+save+", Plot="+plot+", Verbose="+verbose+")\n";

			runPythonScript(pyInput);
		}

		/**
		 * Plots the loaded file to the miniplot
		 * 
		 * @param filename :: The name of the workspace to plot
		 */
		void ResNorm::handleVanadiumInputReady(const QString& filename)
		{
			double res = getInstrumentResolution(filename);
			plotMiniPlot(filename, 0);
			std::pair<double, double> range = getCurveRange();

			if(res != 0)
			{
				m_dblManager->setValue(m_properties["EMin"], -res);
				m_dblManager->setValue(m_properties["EMax"], res);
				setMiniPlotRange(-res, res);
			}
			else
			{
				m_dblManager->setValue(m_properties["EMin"], range.first);
				m_dblManager->setValue(m_properties["EMax"], range.second);
				setMiniPlotRange(range.first, range.second);
			}

			m_dblManager->setMinimum(m_properties["EMin"], range.first);
			m_dblManager->setMaximum(m_properties["EMin"], range.second);
			m_dblManager->setMinimum(m_properties["EMax"], range.first);
			m_dblManager->setMaximum(m_properties["EMax"], range.second);
			m_rangeSelector->setRange(range.first, range.second);
		}

		void ResNorm::minValueChanged(double min)
    {
      m_dblManager->setValue(m_properties["EMin"], min);
    }

    void ResNorm::maxValueChanged(double max)
    {
			m_dblManager->setValue(m_properties["EMax"], max);	
    }

    void ResNorm::updateProperties(QtProperty* prop, double val)
    {
    	if(prop == m_properties["EMin"])
    	{
    		// Check if the user is setting the max less than the min
    		if(val > m_dblManager->value(m_properties["EMax"]))
    		{
    			m_dblManager->setValue(m_properties["EMin"], m_dblManager->value(m_properties["EMax"]));
    		}
    		else
    		{
					m_rangeSelector->setMinimum(val);
    		}
    	}
    	else if (prop == m_properties["EMax"])
    	{
    		// Check if the user is setting the min greater than the max
    		if(val < m_dblManager->value(m_properties["EMin"]))
    		{
    			m_dblManager->setValue(m_properties["EMax"], m_dblManager->value(m_properties["EMin"]));
    		}
    		else
    		{
    			m_rangeSelector->setMaximum(val);
    		}
			}
    }
	} // namespace CustomInterfaces
} // namespace MantidQt
