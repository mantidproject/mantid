#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidQtCustomInterfaces/JumpFit.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <string>
#include <boost/lexical_cast.hpp>

namespace MantidQt
{
	namespace CustomInterfaces
	{
		JumpFit::JumpFit(QWidget * parent) : 
			IndirectBayesTab(parent)
		{
			m_uiForm.setupUi(parent);

			//add the plot to the ui form
			m_uiForm.plotSpace->addWidget(m_plot);
			//add the properties browser to the ui form
			m_uiForm.treeSpace->addWidget(m_propTree);

			m_properties["QMin"] = m_dblManager->addProperty("QMin");
			m_properties["QMax"] = m_dblManager->addProperty("QMax");
			
			m_dblManager->setDecimals(m_properties["QMin"], NUM_DECIMALS);
			m_dblManager->setDecimals(m_properties["QMax"], NUM_DECIMALS);

			m_propTree->addProperty(m_properties["QMin"]);
			m_propTree->addProperty(m_properties["QMax"]);

			m_uiForm.cbWidth->setEnabled(false);

			// Connect data selector to handler method
			connect(m_uiForm.dsSample, SIGNAL(dataReady(const QString&)), this, SLOT(handleSampleInputReady(const QString&)));
			// Connect width selector to handler method
			connect(m_uiForm.cbWidth, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(handleWidthChange(const QString&)));
		}

		/**
		 * Validate the form to check the program can be run
		 * 
		 * @return :: Whether the form was valid
		 */
		bool JumpFit::validate()
		{
			UserInputValidator uiv;
			uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSample);

			//this workspace doesn't have any valid widths
			if(spectraList.size() == 0)
			{
				uiv.addErrorMessage("Input workspace doesn't appear to contain any width data.");
			}

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
		 * script that runs JumpFit
		 */
		void JumpFit::run() 
		{
			QString verbose("False");
			QString plot("False");
			QString save("False");
			
			QString sample = m_uiForm.dsSample->getCurrentDataName();

			//fit function to use
			QString fitFunction("CE");
			switch(m_uiForm.cbFunction->currentIndex())
			{
				case 0:
					fitFunction = "CE"; // Use Chudley-Elliott
					break;
				case 1:
					fitFunction = "HallRoss";
					break;
				case 2:
					fitFunction = "Fick";
					break;
				case 3:
					fitFunction = "Teixeira";
					break;
			}

			std::string widthText = m_uiForm.cbWidth->currentText().toStdString();
			int width = spectraList[widthText];
			QString widthTxt = boost::lexical_cast<std::string>(width).c_str();

			// Cropping values
			QString QMin = m_properties["QMin"]->valueText();
			QString QMax = m_properties["QMax"]->valueText();

			//output options
			if(m_uiForm.chkVerbose->isChecked()) { verbose = "True"; }
			if(m_uiForm.chkSave->isChecked()) { save = "True"; }
			if(m_uiForm.chkPlot->isChecked()) { plot = "True"; }

			QString pyInput = 
				"from IndirectJumpFit import JumpRun\n";

			pyInput += "JumpRun('"+sample+"','"+fitFunction+"',"+widthTxt+","+QMin+","+QMax+","
									"Save="+save+", Plot="+plot+", Verbose="+verbose+")\n";

			runPythonScript(pyInput);
		}

		/**
		 * Set the data selectors to use the default save directory
		 * when browsing for input files.
		 *  
     * @param settings :: The current settings
		 */
		void JumpFit::loadSettings(const QSettings& settings)
		{
			m_uiForm.dsSample->readSettings(settings.group());
		}

		/**
		 * Plots the loaded file to the miniplot and sets the guides
		 * and the range
		 * 
		 * @param filename :: The name of the workspace to plot
		 */
		void JumpFit::handleSampleInputReady(const QString& filename)
		{

			auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(filename.toStdString());
			auto mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);

			findAllWidths(mws);
			
			if(spectraList.size() > 0)
			{
				m_uiForm.cbWidth->setEnabled(true);

				std::string currentWidth = m_uiForm.cbWidth->currentText().toStdString();
				plotMiniPlot(filename, spectraList[currentWidth]);
				std::pair<double,double> res;
				std::pair<double,double> range = getCurveRange();

				//Use the values from the instrument parameter file if we can
				if(getInstrumentResolution(filename, res))
				{
					setMiniPlotGuides(m_properties["QMin"], m_properties["QMax"], res);
				}
				else
				{
					setMiniPlotGuides(m_properties["QMin"], m_properties["QMax"], range);
				}

				setPlotRange(m_properties["QMin"], m_properties["QMax"], range);
			}
			else
			{
				m_uiForm.cbWidth->setEnabled(false);
				emit showMessageBox("Workspace doesn't appear to contain any width data");
			}
		}

		/**
		 * Find all of the spectra in the workspace that have width data 
		 * 
		 * @param ws :: The workspace to search
		 */
		void JumpFit::findAllWidths(Mantid::API::MatrixWorkspace_const_sptr ws)
		{
			m_uiForm.cbWidth->clear();
			spectraList.clear();

			for (size_t i = 0; i < ws->getNumberHistograms(); ++i)
			{
				auto axis = dynamic_cast<Mantid::API::TextAxis*>(ws->getAxis(1));
				std::string title = axis->label(i);

				//check if the axis labels indicate this spectrum is width data 
				size_t qLinesWidthIndex = title.find(".Width");
				size_t convFitWidthIndex = title.find(".FWHM");

				bool qLinesWidth = qLinesWidthIndex != std::string::npos;
				bool convFitWidth = convFitWidthIndex != std::string::npos;

				//if we get a match, add this spectrum to the combobox
				if(convFitWidth || qLinesWidth)
				{
					std::string cbItemName = "";
					size_t substrIndex = 0;
					
					if (qLinesWidth)
					{
						substrIndex = qLinesWidthIndex;
					}
					else if (convFitWidth)
					{
						substrIndex = convFitWidthIndex;
					}

					cbItemName = title.substr(0, substrIndex);
					spectraList[cbItemName] = static_cast<int>(i);
					m_uiForm.cbWidth->addItem(QString(cbItemName.c_str()));
					
					//display widths f1.f1, f2.f1 and f2.f2
					if (m_uiForm.cbWidth->count() == 3)
					{
						return;
					}
				}
			}
		}

		/**
		 * Plots the loaded file to the miniplot when the selected spectrum changes
		 * 
		 * @param text :: The name spectrum index to plot
		 */
		void JumpFit::handleWidthChange(const QString& text)
		{
			QString sampleName = m_uiForm.dsSample->getCurrentDataName();
			QString samplePath = m_uiForm.dsSample->getFullFilePath();

			if(!sampleName.isEmpty() && spectraList.size() > 0)
			{
				if(validate())
				{
					plotMiniPlot(sampleName, spectraList[text.toStdString()]);
				}
			}
		}

		/**
		 * Updates the property manager when the lower guide is moved on the mini plot
		 *
		 * @param min :: The new value of the lower guide
		 */
		void JumpFit::minValueChanged(double min)
    {
      m_dblManager->setValue(m_properties["QMin"], min);
    }

		/**
		 * Updates the property manager when the upper guide is moved on the mini plot
		 *
		 * @param max :: The new value of the upper guide
		 */
    void JumpFit::maxValueChanged(double max)
    {
			m_dblManager->setValue(m_properties["QMax"], max);	
    }

		/**
		 * Handles when properties in the property manager are updated.
		 *
		 * @param prop :: The property being updated
		 * @param val :: The new value for the property
		 */
    void JumpFit::updateProperties(QtProperty* prop, double val)
    {
    	if(prop == m_properties["QMin"])
    	{
    		updateLowerGuide(m_properties["QMin"], m_properties["QMax"], val);
    	}
    	else if (prop == m_properties["QMax"])
    	{
				updateUpperGuide(m_properties["QMin"], m_properties["QMax"], val);
    	}
    }
	} // namespace CustomInterfaces
} // namespace MantidQt
