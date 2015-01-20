#include "MantidQtCustomInterfaces/ResNorm.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

namespace MantidQt
{
	namespace CustomInterfaces
	{
		ResNorm::ResNorm(QWidget * parent) : 
			IndirectBayesTab(parent)
		{
			m_uiForm.setupUi(parent);

			// Create the plot
      m_plots["ResNormPlot"] = new QwtPlot(m_parentWidget);
      m_plots["ResNormPlot"]->setCanvasBackground(Qt::white);
      m_plots["ResNormPlot"]->setAxisFont(QwtPlot::xBottom, parent->font());
      m_plots["ResNormPlot"]->setAxisFont(QwtPlot::yLeft, parent->font());
			m_uiForm.plotSpace->addWidget(m_plots["ResNormPlot"]);

      // Create range selector
      m_rangeSelectors["ResNormERange"] = new MantidWidgets::RangeSelector(m_plots["ResNormPlot"]);
      connect(m_rangeSelectors["ResNormERange"], SIGNAL(minValueChanged(double)), this, SLOT(minValueChanged(double)));
      connect(m_rangeSelectors["ResNormERange"], SIGNAL(maxValueChanged(double)), this, SLOT(maxValueChanged(double)));

			// Add the properties browser to the ui form
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

			// Connect data selector to handler method
			connect(m_uiForm.dsVanadium, SIGNAL(dataReady(const QString&)), this, SLOT(handleVanadiumInputReady(const QString&)));
		}

    void ResNorm::setup()
    {
    }

		/**
		 * Validate the form to check the program can be run
		 * 
		 * @return :: Whether the form was valid
		 */
		bool ResNorm::validate()
		{
			UserInputValidator uiv;
			uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsVanadium);
			uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

			QString errors = uiv.generateErrorMessage();
			if (!errors.isEmpty())
			{
				emit showMessageBox(errors);
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
			QString save("False");

			QString pyInput = 
				"from IndirectBayes import ResNormRun\n";

			// get the file names
			QString VanName = m_uiForm.dsVanadium->getCurrentDataName();
			QString ResName = m_uiForm.dsResolution->getCurrentDataName();

			// get the parameters for ResNorm
			QString EMin = m_properties["EMin"]->valueText();
			QString EMax = m_properties["EMax"]->valueText();

			QString ERange = "[" + EMin + "," + EMax + "]";

			QString nBin = m_properties["VanBinning"]->valueText();

			// get output options
			if(m_uiForm.chkSave->isChecked()){ save ="True"; }
			QString plot = m_uiForm.cbPlot->currentText();

			pyInput += "ResNormRun('"+VanName+"', '"+ResName+"', "+ERange+", "+nBin+","
										" Save="+save+", Plot='"+plot+"', Verbose=True)\n";

			runPythonScript(pyInput);
		}

		/**
		 * Set the data selectors to use the default save directory
		 * when browsing for input files.
		 *  
     * @param settings :: The current settings
		 */
		void ResNorm::loadSettings(const QSettings& settings)
		{
			m_uiForm.dsVanadium->readSettings(settings.group());
    	m_uiForm.dsResolution->readSettings(settings.group());
		}

		/**
		 * Plots the loaded file to the miniplot and sets the guides
		 * and the range
		 * 
		 * @param filename :: The name of the workspace to plot
		 */
		void ResNorm::handleVanadiumInputReady(const QString& filename)
		{
			plotMiniPlot(filename, 0, "ResNormPlot", "RawPlotCurve");
			std::pair<double,double> res;
			std::pair<double,double> range = getCurveRange("RawPlotCurve");

			//Use the values from the instrument parameter file if we can
			if(getInstrumentResolution(filename, res))
			{
				//ResNorm resolution should be +/- 10 * the IPF resolution
				res.first = res.first * 10;
				res.second = res.second * 10;

				setMiniPlotGuides("ResNormERange", m_properties["EMin"], m_properties["EMax"], res);
			}
			else
			{
				setMiniPlotGuides("ResNormERange", m_properties["EMin"], m_properties["EMax"], range);
			}

			setPlotRange("ResNormERange", m_properties["EMin"], m_properties["EMax"], range);
		}

		/**
		 * Updates the property manager when the lower guide is moved on the mini plot
		 *
		 * @param min :: The new value of the lower guide
		 */
		void ResNorm::minValueChanged(double min)
    {
      m_dblManager->setValue(m_properties["EMin"], min);
    }

		/**
		 * Updates the property manager when the upper guide is moved on the mini plot
		 *
		 * @param max :: The new value of the upper guide
		 */
    void ResNorm::maxValueChanged(double max)
    {
			m_dblManager->setValue(m_properties["EMax"], max);	
    }

		/**
		 * Handles when properties in the property manager are updated.
		 *
		 * @param prop :: The property being updated
		 * @param val :: The new value for the property
		 */
    void ResNorm::updateProperties(QtProperty* prop, double val)
    {
    	if(prop == m_properties["EMin"])
    	{
				updateLowerGuide(m_rangeSelectors["ResNormERange"], m_properties["EMin"], m_properties["EMax"], val);
    	}
    	else if (prop == m_properties["EMax"])
    	{
    		updateUpperGuide(m_rangeSelectors["ResNormERange"], m_properties["EMin"], m_properties["EMax"], val);
			}
    }
	} // namespace CustomInterfaces
} // namespace MantidQt
