#include "MantidQtCustomInterfaces/Indirect.h"

#include "MantidQtCustomInterfaces/Background.h"

#include "MantidKernel/ConfigService.h"

#include <QUrl>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>

using namespace MantidQt::CustomInterfaces;

/**
* This is the constructor for the Indirect Instruments Interface.
* It is used primarily to ensure sane values for member variables.
*/
Indirect::Indirect(QWidget *parent, Ui::ConvertToEnergy & uiForm) : 
UserSubWindow(parent), m_uiForm(uiForm), m_backgroundDialog(NULL)
{
	// Constructor
}

/**
* This function performs any one-time actions needed when the Inelastic interface
* is first selected, such as connecting signals to slots.
*/
void Indirect::initLayout()
{
	// connect Indirect-specific signals (buttons,checkboxes,etc) to suitable slots.

	// "Energy Transfer" tab
	connect(m_uiForm.cbAnalyser, SIGNAL(activated(int)), this, SLOT(analyserSelected(int)));
	connect(m_uiForm.cbReflection, SIGNAL(activated(int)), this, SLOT(reflectionSelected(int)));
	connect(m_uiForm.cbMappingOptions, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(mappingOptionSelected(const QString&)));

	connect(m_uiForm.pbBack_2, SIGNAL(clicked()), this, SLOT(backgroundClicked()));
	connect(m_uiForm.pbPlotRaw, SIGNAL(clicked()), this, SLOT(plotRaw()));
	connect(m_uiForm.rebin_pbRebin, SIGNAL(clicked()), this, SLOT(rebinData()));

	// "Browse" buttons
	connect(m_uiForm.pbRunFilesBrowse, SIGNAL(clicked()), this, SLOT(browseRun()));
	connect(m_uiForm.pbCalibrationFilesBrowse, SIGNAL(clicked()), this, SLOT(browseCalib()));
	connect(m_uiForm.pbMappingFileBrowse, SIGNAL(clicked()), this, SLOT(browseMap()));
	connect(m_uiForm.pbBrowseSPE, SIGNAL(clicked()), this, SLOT(browseSave()));

	// "Calibration" tab
	connect(m_uiForm.cal_pbPlot, SIGNAL(clicked()), this, SLOT(calibPlot()));
	connect(m_uiForm.cal_pbCreate, SIGNAL(clicked()), this, SLOT(calibCreate()));

	// set values of m_dataDir and m_saveDir
	m_dataDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"));
	m_saveDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

}

/**
* This function will hold any Python-dependent setup actions for the interface.
*/
void Indirect::initLocalPython()
{
	//
}

/**
* This function opens a web browser window to the Mantid Project wiki page for this
* interface ("Inelastic" subsection of ConvertToEnergy).
*/
void Indirect::helpClicked()
{
	QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
		"ConvertToEnergy#Inelastic"));
}

/**
* This function will control the actions needed for the Indirect interface when the
* "Run" button is clicked by the user.
*/
void Indirect::runClicked()
{
	QString groupFile = createMapFile(m_uiForm.cbMappingOptions->currentText());
	if ( groupFile == "" )
	{
		return;
	}

	QString filePrefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString().toLower();
	filePrefix += "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText() + "_";

	QString pyInput =
		"from mantidsimple import *\n"
		"# Set up variables\n"
		"filename = r'" +m_uiForm.leRunFiles->text()+ "'\n"
		"first = " +m_uiForm.leSpectraMin->text()+ "\n"
		"last = " +m_uiForm.leSpectraMax->text()+ "\n"
		"efixed = " +m_uiForm.leEfixed->text()+ "\n"
		"inWS = 'Mon_in'\n"
		"try:\n"
		"   LoadRaw(filename, inWS, SpectrumMin = 1, SpectrumMax = 1)\n"
		"except SystemExit:\n"
		"   print 'Could not open .raw file.'\n"
		"   sys.exit('Could not open .raw file.')\n"
		"outWS = 'TimeRegime'\n"
		"LoadRaw(filename, outWS, SpectrumMin = 1, SpectrumMax = 3)\n"
		"workspace = mantid.getMatrixWorkspace(outWS)\n"
		"SpecA = workspace.readX(0)[0]\n"
		"SpecB = workspace.readX(2)[0]\n"
		"mantid.deleteWorkspace(outWS)\n"
		"monWS = 'Mon'\n"
		"if ( SpecA == SpecB ): # Single Monitor (?)\n"
		"   alg = Unwrap(inWS, monWS, LRef = '37.86')\n"
		"   join = float(alg.getPropertyValue('JoinWavelength'))\n"
		"   RemoveBins(monWS, monWS, join-0.001, join+0.001, Interpolation='Linear')\n"
		"   FFTSmooth(monWS, monWS, 0)\n"
		"else: # Multiple Monitor (?)\n"
		"   ConvertUnits(inWS, monWS, 'Wavelength')\n"
		"mantid.deleteWorkspace(inWS)\n";
	if ( m_uiForm.ckMonEff->isChecked() )
	{
		pyInput +=
			"#Monitor Efficiency\n"
			"CreateSingleValuedWorkspace('moneff', 1.276e-3)\n"
			"OneMinusExponentialCor(monWS, monWS, (8.3 * 0.025) )\n"
			"Divide(monWS,'moneff',monWS)\n"
			"mantid.deleteWorkspace('moneff')\n";
	}
	pyInput +=
		"inWS = 'Time'\n"
		"LoadRaw(filename, inWS, SpectrumMin = first, SpectrumMax = last)\n";
	if ( m_uiForm.ckUseCalib->isChecked() )
	{
		if ( m_uiForm.leCalibrationFile->text() == "" )
		{
			showInformationBox("Please enter path to calibration file.");
			return;
		}
		pyInput +=
			"# Use Calibration File\n"
			"calib = r\"" +m_uiForm.leCalibrationFile->text()+ "\"\n"
			"try:\n"
			"   LoadNexusProcessed(calib, 'calib')\n" // Calibration File Path goes here
			"except SystemExit:\n"
			"   print 'Could not open calibration file. Please check file path is correct.'\n"
			"   sys.exit('Could not open calibration file.')\n"
			"tmp = mantid.getMatrixWorkspace(inWS)\n"
			"shist = tmp.getNumberHistograms()\n"
			"tmp = mantid.getMatrixWorkspace('calib')\n"
			"chist = tmp.getNumberHistograms()\n"
			"if chist != shist:\n"
			"	print 'Number of spectra in calibration file does not match data file.'\n"
			"	mantid.deleteWorkspace('calib')\n"
			"	sys.exit('Number of spectra in calibration file does not match data file.')\n"
			"else:\n"
			"	Divide(inWS,'calib',inWS)\n"
			"	mantid.deleteWorkspace('calib')\n";
	}
	pyInput +=
		"enWS = 'Energy'\n"
		"# Normalise to Monitor\n"
		"ConvertUnits(inWS,enWS, 'Wavelength')\n"
		"RebinToWorkspace(enWS,monWS,enWS)\n"
		"Divide(enWS,monWS,enWS)\n"
		"ConvertUnits(enWS,enWS,'DeltaE','Indirect',efixed)\n"
		"mantid.deleteWorkspace(monWS)\n"
		"mantid.deleteWorkspace(inWS)\n";
	if ( m_uiForm.ckDetailedBalance->isChecked() )
	{
		pyInput +=
			"# Detailed Balance\n"
			"ExponentialCorrection(enWS, enWS, 1.0, (11.606 / ( 2 * 300 ) ) )\n";
	}
	pyInput +=
		"scaleWS = 'scale'\n"
		"scale = 1e9\n"
		"CreateSingleValuedWorkspace(scaleWS, scale)\n"
		"Multiply(enWS, scaleWS, enWS)\n"
		"mantid.deleteWorkspace(scaleWS)\n";

	pyInput +=
		"iconWS = '" +m_uiForm.cbAnalyser->currentText() + "'\n"
		"GroupDetectors(enWS, iconWS, MapFile=r'" + groupFile +"')\n"
		"mantid.deleteWorkspace(enWS)\n";
	if ( m_uiForm.leNameSPE->text() != "" )
	{
		pyInput += "savefile = r'" +m_uiForm.leNameSPE->text()+ "'\n";
	}
	else {
		pyInput +=
			"savefile = mtd.getConfigProperty('defaultsave.directory')\n"
			"savefile += '" +filePrefix+ "'\n";
		if ( m_uiForm.cbAnalyser->currentText() == "graphite" )
		{
			pyInput += "savefile += 'ipg.nxs'\n";
		}
		else if ( m_uiForm.cbAnalyser->currentText() == "mica" || m_uiForm.cbAnalyser->currentText() == "fmica" )
		{
			pyInput += "savefile += 'imi.nxs'\n";
		}
	}
	pyInput +=
		"SaveNexusProcessed(iconWS, savefile)\n";

	QString pyOutput = runPythonCode(pyInput).trimmed();

	if ( pyOutput != "" )
	{
		showInformationBox("The following error occurred:\n" + pyOutput
			+ "\n\nAnalysis did not complete.");
	}

}

/**
* This function holds any steps that must be performed on the selection of an instrument,
* for example loading values from the Instrument Definition File (IDF).
* @param prefix The selected instruments prefix in Mantid.
*/
void Indirect::setIDFValues(const QString & prefix)
{
	// empty ComboBoxes, LineEdits,etc of previous values
	m_uiForm.cbAnalyser->clear();
	m_uiForm.cbReflection->clear();
	clearReflectionInfo();

	// Get list of analysers and populate cbAnalyser
	QString pyInput = 
		"from mantidsimple import *\n"
		"LoadEmptyInstrument(\"%1\", \"ins\")\n"
		"workspace = mtd['ins']\n"
		"instrument = workspace.getInstrument()\n"
		"ana_list_split = instrument.getStringParameter(\"analysers\")[0].split(\",\")\n"
		"reflections = []\n"
		"for i in range(0,len(ana_list_split)):\n"
		"   list = []\n"
		"   name = \"refl-\" +ana_list_split[i]\n"
		"   list.append( ana_list_split[i] )\n"
		"   try:\n"
		"      item = instrument.getStringParameter(name)[0]\n"
		"   except IndexError:\n"
		"      item = \"\"\n"
		"   refl = item.split(\",\")\n"
		"   list.append( refl )\n"
		"   reflections.append(list)\n"
		"for i in range(0, len(reflections)):\n"
		"   message = reflections[i][0] + \"-\"\n"
		"   for j in range(0,len(reflections[i][1])):\n"
		"      message += str(reflections[i][1][j])\n"
		"      if j < ( len(reflections[i][1]) -1 ):\n"
		"         message += \",\"\n"
		"   print message\n"
		"mtd.deleteWorkspace(\"ins\")\n";

	QString defFile = getIDFPath(m_uiForm.cbInst->currentText());
	if ( defFile == "" )
	{
		showInformationBox("Could not locate IDF.");
		return;
	}

	getSpectraRanges(defFile);

	pyInput = pyInput.arg(defFile);
	QString pyOutput = runPythonCode(pyInput).trimmed();

	if ( pyOutput == "" )
	{
		showInformationBox("Could not get list of analysers from Instrument Parameter file.");
		return;
	}

	QStringList analysers = pyOutput.split("\n", QString::SkipEmptyParts);

	for (int i = 0; i< analysers.count(); i++ )
	{
		QString text; // holds Text field of combo box (name of analyser)
		QVariant data; // holds Data field of combo box (list of reflections)

		QStringList analyser = analysers[i].split("-", QString::SkipEmptyParts);

		text = analyser[0];

		if ( analyser.count() > 1 )
		{
			QStringList reflections = analyser[1].split(",", QString::SkipEmptyParts);
			data = QVariant(reflections);
			m_uiForm.cbAnalyser->addItem(text, data);
		}
		else
		{
			m_uiForm.cbAnalyser->addItem(text);
		}
	}

	analyserSelected(m_uiForm.cbAnalyser->currentIndex());


}


/**
* Gets the path to the selected instrument's Instrument Definition File (IDF), if the instrument has a parameter file.
* @param name the instrument's name from the QComboBox
* @return A string containing the path to the IDF, or an empty string if no parameter file exists.
*/
QString Indirect::getIDFPath(const QString& name)
{
	QString paramfile_dir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("parameterDefinition.directory"));
	QDir paramdir(paramfile_dir);
	paramdir.setFilter(QDir::Files);
	QStringList filters;
	filters << name + "*_Definition.xml";
	paramdir.setNameFilters(filters);

	QStringList entries = paramdir.entryList();
	QString defFilePrefix;

	if( entries.isEmpty() )
	{
		return "";
	}
	else
	{
		defFilePrefix = entries[0];
	}

	QString defFile = paramdir.filePath(defFilePrefix);
	return defFile;
}

/**
* This function loads the min and max values for the analysers spectra and
* displays this in the "Calibration" tab.
* @param defFile path to instrument definition file.
*/
void Indirect::getSpectraRanges(const QString& defFile)
{
	QString pyInput =
		"from mantidsimple import *\n"
		"LoadEmptyInstrument(\"%1\", \"ins\")\n"
		"workspace = mtd['ins']\n"
		"instrument = workspace.getInstrument()\n"
		"analyser = []\n"
		"analyser_final = []\n"
		"for i in range(0, instrument.nElements() ):\n"
		"	if instrument[i].type() == \"ParCompAssembly\":\n"
		"		analyser.append(instrument[i])\n"
		"for i in range(0, len(analyser) ):\n"
		"	analyser_final.append(analyser[i])\n"
		"	for j in range(0, analyser[i].nElements() ):\n"
		"		if analyser[i][j].type() == \"ParCompAssembly\":\n"
		"			try:\n"
		"				analyser_final.remove(analyser[i])\n"
		"			except ValueError:\n"
		"				pass\n"
		"			analyser_final.append(analyser[i][j])\n"
		"for i in range(0, len(analyser_final)):\n"
		"	message = analyser_final[i].getName() + \"-\"\n"
		"	message += str(analyser_final[i][0].getID()) + \",\"\n"
		"	message += str(analyser_final[i][analyser_final[i].nElements()-1].getID())\n"
		"	print message\n"
		"mtd.deleteWorkspace(\"ins\")\n";

	pyInput = pyInput.arg(defFile);

	QString pyOutput = runPythonCode(pyInput).trimmed();

	if ( pyOutput == "" )
	{
		showInformationBox("Could not gather Spectral Ranges from IDF.");
		return;
	}

	QStringList analysers = pyOutput.split("\n", QString::SkipEmptyParts);

	QLabel* lbPGMin = m_uiForm.cal_lbGraphiteMin;
	QLabel* lbPGMax = m_uiForm.cal_lbGraphiteMax;
	QLabel* lbMiMin = m_uiForm.cal_lbMicaMin;
	QLabel* lbMiMax = m_uiForm.cal_lbMicaMax;
	QLabel* lbDifMin = m_uiForm.cal_lbDiffractionMin;
	QLabel* lbDifMax = m_uiForm.cal_lbDiffractionMax;

	for ( int i = 0 ; i < analysers.count() ; i++ )
	{
		QStringList analyser_spectra = analysers[i].split("-", QString::SkipEmptyParts);
		QStringList first_last = analyser_spectra[1].split(",", QString::SkipEmptyParts);
		if(  analyser_spectra[0] == "graphite" )
		{
			lbPGMin->setText(first_last[0]);
			lbPGMax->setText(first_last[1]);
		}
		else if ( analyser_spectra[0] == "mica" )
		{
			lbMiMin->setText(first_last[0]);
			lbMiMax->setText(first_last[1]);
		}
		else if ( analyser_spectra[0] == "diffraction" )
		{
			lbDifMin->setText(first_last[0]);
			lbDifMax->setText(first_last[1]);
		}

	}
}

/**
* This function clears the values of the QLineEdit objects used to hold Reflection-specific
* information.
*/
void Indirect::clearReflectionInfo()
{
	m_uiForm.leSpectraMin->clear();
	m_uiForm.leSpectraMax->clear();
	m_uiForm.leEfixed->clear();
	m_uiForm.cal_lePeakMin->clear();
	m_uiForm.cal_lePeakMax->clear();
	m_uiForm.cal_leBackMin->clear();
	m_uiForm.cal_leBackMax->clear();
}

/**
* This function creates the mapping/grouping file for the data analysis.
* @param groupType Type of grouping (All, Group, Indiviual)
* @return path to mapping file, or an empty string if file could not be created.
*/
QString Indirect::createMapFile(const QString& groupType)
{

	QString groupFile, ngroup, nspec;
	QString ndet = "( "+m_uiForm.leSpectraMax->text()+" - "+m_uiForm.leSpectraMin->text()+") + 1";

	if ( groupType == "File" )
	{
		groupFile = m_uiForm.leMappingFile->text();
		if ( groupFile == "" )
		{
			showInformationBox("You must enter a path to the .map file.");
		}
		return groupFile;
	}
	else if ( groupType == "Groups" )
	{
		ngroup = m_uiForm.leNoGroups->text();
		nspec = "( " +ndet+ " ) / " +ngroup;
	}
	else if ( groupType == "All" )
	{
		ngroup = "1";
		nspec = ndet;
	}
	else if ( groupType == "Individual" )
	{
		ngroup = ndet;
		nspec = "1";
	}

	groupFile = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString().toLower();
	groupFile += "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText();
	groupFile += "_" + groupType + ".map";	

	QString pyInput =
		"filename = mtd.getConfigProperty('defaultsave.directory')\n"
		"filename += \"" +groupFile+ "\"\n"
		"handle = open(filename, 'w')\n" // open file
		"ngroup = " +ngroup+ "\n" // set values
		"nspec = " +nspec+ "\n"
		"first = " +m_uiForm.leSpectraMin->text()+ "\n"
		"handle.write(str(ngroup) +  \"\\n\" )\n"
		"for n in range(0, ngroup):\n"
		"   n1 = n * nspec + first\n"
		"   handle.write(str(n+1) +  \"\\n\" )\n" // group number
		"   handle.write(str(nspec) +  \"\\n\" )\n" // number of spectra in group
		"   for i in range(1, nspec+1):\n"
		"      n3 = n1 + i - 1\n"
		"      handle.write(str(n3).center(4) + \" \")\n"
		"   handle.write(\"\\n\")\n"
		"handle.close()\n"
		"print filename\n";

	QString pyOutput = runPythonCode(pyInput).trimmed();

	return pyOutput;
}

/**
* This function is called when the user selects an analyser from the cbAnalyser QComboBox
* object. It's main purpose is to initialise the values for the Reflection ComboBox.
* @param index Index of the value selected in the combo box.
*/
void Indirect::analyserSelected(int index)
{
	// populate Reflection combobox with correct values for Analyser selected.
	m_uiForm.cbReflection->clear();
	clearReflectionInfo();


	QVariant currentData = m_uiForm.cbAnalyser->itemData(index);
	if ( currentData == QVariant::Invalid )
	{
		m_uiForm.lbReflection->setEnabled(false);
		m_uiForm.cbReflection->setEnabled(false);
		return;
	}
	else
	{
		m_uiForm.lbReflection->setEnabled(true);
		m_uiForm.cbReflection->setEnabled(true);
		QStringList reflections = currentData.toStringList();
		for ( int i = 0; i < reflections.count(); i++ )
		{
			m_uiForm.cbReflection->addItem(reflections[i]);
		}
	}

	reflectionSelected(m_uiForm.cbReflection->currentIndex());
}


/**
* This function is called when the user selects a reflection from the cbReflection QComboBox
* object.
* @param index Index of the value selected in the combo box.
*/
void Indirect::reflectionSelected(int index)
{
	// first, clear values in assosciated boxes:
	clearReflectionInfo();

	QString defFile = getIDFPath(m_uiForm.cbInst->currentText());
	QString paramFile = defFile;
	paramFile.chop(14);
	paramFile +=
		m_uiForm.cbAnalyser->currentText() + "_" +
		m_uiForm.cbReflection->currentText() + "_Parameters.xml";

	QString pyInput =
		"from mantidsimple import *\n"
		"LoadEmptyInstrument(\"%1\", \"ins\")\n"
		"LoadParameterFile(\"ins\", \"%2\")\n"
		"instrument = mtd['ins'].getInstrument()\n"
		"print int(instrument.getNumberParameter(\"spectra-min\")[0])\n"
		"print int(instrument.getNumberParameter(\"spectra-max\")[0])\n"
		"print instrument.getNumberParameter(\"efixed-val\")[0]\n"
		"print int(instrument.getNumberParameter(\"peak-start\")[0])\n"
		"print int(instrument.getNumberParameter(\"peak-end\")[0])\n"
		"print int(instrument.getNumberParameter(\"back-start\")[0])\n"
		"print int(instrument.getNumberParameter(\"back-end\")[0])\n"
		"mtd.deleteWorkspace(\"ins\")\n";

	pyInput = pyInput.arg(defFile);
	pyInput = pyInput.arg(paramFile);

	QString pyOutput = runPythonCode(pyInput).trimmed();

	QStringList values = pyOutput.split("\n", QString::SkipEmptyParts);

	if ( values.count() != 7 )
	{
		showInformationBox("Could not gather necessary data from parameter file.");
		return;
	}


	m_uiForm.leSpectraMin->setText(values[0]);
	m_uiForm.leSpectraMax->setText(values[1]);
	m_uiForm.leEfixed->setText(values[2]);
	m_uiForm.cal_lePeakMin->setText(values[3]);
	m_uiForm.cal_lePeakMax->setText(values[4]);
	m_uiForm.cal_leBackMin->setText(values[5]);
	m_uiForm.cal_leBackMax->setText(values[6]);
}

/**
* This function runs when the user makes a selection on the cbMappingOptions QComboBox.
* @param groupType Value of selection made by user.
*/
void Indirect::mappingOptionSelected(const QString& groupType)
{
	if ( groupType == "File" )
	{
		m_uiForm.swMapping->setCurrentIndex(0);
		m_uiForm.swMapping->setEnabled(true);
	}
	else if ( groupType == "All" )
	{
		m_uiForm.swMapping->setCurrentIndex(2);
		m_uiForm.swMapping->setEnabled(false);
	}
	else if ( groupType == "Individual" )
	{
		m_uiForm.swMapping->setCurrentIndex(2);
		m_uiForm.swMapping->setEnabled(false);
	}
	else if ( groupType == "Groups" )
	{
		m_uiForm.swMapping->setCurrentIndex(1);
		m_uiForm.swMapping->setEnabled(true);
	}
}

/**
* This function is called when a user clicks on the "Browse" button assosciated with
* the "Run Files" section.
*/
void Indirect::browseRun()
{
	QString runFile = QFileDialog::getOpenFileName(this, "Select RAW Data",
		m_dataDir, "ISIS Raw Files (*.raw)");
	m_uiForm.leRunFiles->setText(runFile);
}

/**
* As above, but for the Calibration file.
*/
void Indirect::browseCalib()
{
	QString calFile = QFileDialog::getOpenFileName(this, "Select Calibration File",
		m_dataDir, "Calib Files (*calib.nxs)");
	m_uiForm.leCalibrationFile->setText(calFile);
	if ( calFile != "" )
	{
		m_uiForm.ckUseCalib->setChecked(true);
	}
}
/**
* Again, as above but for the Mapping File.
*/
void Indirect::browseMap()
{
	QString mapFile = QFileDialog::getOpenFileName(this, "Select Mapping / Grouping File",
		m_dataDir, "Spectra Mapping File (*.map)");
	m_uiForm.leMappingFile->setText(mapFile);
}

/**
* Select location to save file.
*/
void Indirect::browseSave()
{
	QString Analyser = m_uiForm.cbAnalyser->currentText();
	QString filePrefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString().toLower();
	filePrefix += "_" + Analyser + m_uiForm.cbReflection->currentText() + "_";

	QString defSave = m_saveDir + filePrefix;
	if ( Analyser == "graphite" )
		defSave += "ipg.nxs";
	else
		defSave += "imi.nxs";
	QString savFile = QFileDialog::getSaveFileName(this, "Save File Name",
		defSave, "Nexus File (*.nxs);;SPE File (*.spe)");

	m_uiForm.leNameSPE->setText(savFile);
}

/**
* This function is called when the user clicks on the Background Removal button. It
* displays the Background Removal dialog, initialising it if it hasn't been already.
*/
void Indirect::backgroundClicked()
{
	if ( m_backgroundDialog == NULL )
	{
		m_backgroundDialog = new Background(this);
		m_backgroundDialog->show();
	}
	else
	{
		m_backgroundDialog->show();
	}
}

/**
* Plots raw time data from .raw file before any data conversion has been performed.
*/
void Indirect::plotRaw()
{
	QString rawFile = m_uiForm.leRunFiles->text();
	if ( rawFile == "" )
	{
		showInformationBox("Please enter the path for the .raw file.");
		return;
	}

	QString pyInput =
		"from mantidsimple import *\n"
		"from mantidplot import *\n"
		"try:\n"
		"   LoadRaw(r'" + rawFile + "', 'RawTime')\n"
		"except SystemExit:\n"
		"   print 'Could not open .raw file. Please check file path.'\n"
		"   sys.exit('Could not open .raw file.')\n"
		"graph = plotSpectrum('RawTime', 0)\n";
	QString pyOutput = runPythonCode(pyInput).trimmed();

	if ( pyOutput != "" )
	{
		showInformationBox(pyOutput);
	}
}

/**
*/
void Indirect::rebinData()
{
	showInformationBox("Not yet implemented.");
}

/**
* This function plots the raw data entered onto the "Calibration" tab, without performing any of the data
* modification steps.
*/
void Indirect::calibPlot()
{
	QString runNo = m_uiForm.cal_leRunNo->displayText();
	if ( runNo == "" )
	{
		showInformationBox("Please enter a run number.");
	}

	QString prefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString().toLower();
	QString input_path = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getDataSearchDirs()[0]);
	input_path += prefix + runNo + ".raw";

	QString pyInput =
		"from mantidsimple import *\n"
		"from mantidplot import *\n"
		"try:\n"
		"   LoadRaw(r'%1', 'Raw', SpectrumMin=%2, SpectrumMax=%3)\n"
		"except:\n"
		"   print 'Could not load .raw file. Please check run number.'\n"
		"   sys.exit('Could not load .raw file.')\n"
		"graph = plotSpectrum(\"Raw\", 0)\n";

	pyInput = pyInput.arg(input_path); // %1 = path to data search directory
	pyInput = pyInput.arg(m_uiForm.leSpectraMin->text()); // %2 = spectra min value
	pyInput = pyInput.arg(m_uiForm.leSpectraMax->text()); // %3 = spectra max value

	QString pyOutput = runPythonCode(pyInput).trimmed();

	if ( pyOutput != "" )
	{
		showInformationBox("Could not load .raw file. Please check run number.");
	}
}

/**
* This function is called when the user clicks on the "Create Calibration File" button.
* Pretty much does what it says on the tin.
*/
void Indirect::calibCreate()
{
	QString runNo = m_uiForm.cal_leRunNo->displayText();
	if ( runNo == "" )
	{
		showInformationBox("Please input a run number.");
		return;
	}

	QString prefix = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString();

	QString output_dir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getOutputDir());

	std::vector<std::string> dataDirs = Mantid::Kernel::ConfigService::Instance().getDataSearchDirs();
	QStringList dataSearchDirs;

	for ( int i = 0; i < dataDirs.size() ; i++ )
	{
		dataSearchDirs.append(QString::fromStdString(dataDirs[i]));
	}

	QString xRange = 
		"[ " + m_uiForm.cal_lePeakMin->text() + ", "
		+ m_uiForm.cal_lePeakMax->text() + ", "
		+ m_uiForm.cal_leBackMin->text() + ", "
		+ m_uiForm.cal_leBackMax->text() + "]";

	QString input_path = dataSearchDirs[0] + prefix + runNo + ".raw";
	QString output_path = output_dir + prefix.toLower() + runNo + "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText() + "_calib.nxs";

	QString pyInput =
		"from mantidsimple import *\n"
		"from mantidplot import *\n"
		"try:\n"
		"   LoadRaw(r\"%1\", \"Raw\", SpectrumMin=%3, SpectrumMax=%4)\n"
		"except ValueError:\n"
		"   print \"Could not load .raw file. Please check run number.\"\n"
		"   sys.exit(0)\n"
		"tmp = mantid.getMatrixWorkspace(\"Raw\")\n"
		"nhist = tmp.getNumberHistograms() - 1\n"
		"xRange = " + xRange + "\n"
		"Integration(\"Raw\", \"Time1\", xRange[0], xRange[1], 0, nhist)\n"
		"Integration(\"Raw\", \"Time2\", xRange[2], xRange[3], 0, nhist)\n"
		"Minus(\"Time1\", \"Time2\", \"Time\")\n"
		"mantid.deleteWorkspace(\"Raw\")\n"
		"mantid.deleteWorkspace(\"Time1\")\n"
		"mantid.deleteWorkspace(\"Time2\")\n"
		"SaveNexusProcessed(\"Time\", r\"%2\", \"Vanadium\")\n";

	if ( m_uiForm.cal_ckPlotResult->isChecked() )
	{ // plot graph of Calibration result if requested by user.
		pyInput +=	"graph = plotTimeBin(\"Time\", 0)\n";
	}
	else
	{ // if graph is not wanted, remove the workspace
		pyInput += "mantid.deleteWorkspace(\"Time\")\n";
	}

	pyInput = pyInput.arg(input_path); // %1 = path to data search directory
	pyInput = pyInput.arg(output_path); // %2 = path to output directory (where to save the file)
	pyInput = pyInput.arg(m_uiForm.leSpectraMin->text()); // %3 = spectra min value
	pyInput = pyInput.arg(m_uiForm.leSpectraMax->text()); // %4 = spectra max value

	QString pyOutput = runPythonCode(pyInput).trimmed();

	if ( pyOutput != "" )
	{
		showInformationBox("Errors:\n" + pyOutput);
		return;
	}

	m_uiForm.leCalibrationFile->setText(output_path);
	m_uiForm.ckUseCalib->setChecked(true);

}
