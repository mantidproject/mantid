#include "MantidQtCustomInterfaces/IndirectConvertToEnergy.h"

#include "MantidQtCustomInterfaces/Background.h"

#include <QInputDialog>

namespace MantidQt
{
namespace CustomInterfaces
{
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  IndirectConvertToEnergy::IndirectConvertToEnergy(Ui::IndirectDataReduction& uiForm, QWidget * parent) :
      IndirectDataReductionTab(uiForm, parent), m_backgroundDialog(NULL), m_bgRemoval(false)
  {
    // Add validators to UI form
    m_uiForm.leScaleMultiplier->setValidator(m_valPosDbl);
    m_uiForm.leNoGroups->setValidator(m_valInt);
    m_uiForm.leDetailedBalance->setValidator(m_valPosDbl);

    m_uiForm.leSpectraMin->setValidator(m_valInt);
    m_uiForm.leSpectraMax->setValidator(m_valInt);

    m_uiForm.entryRebinLow->setValidator(m_valDbl);
    m_uiForm.entryRebinWidth->setValidator(m_valDbl);
    m_uiForm.entryRebinHigh->setValidator(m_valDbl);
    
    // SIGNAL/SLOT CONNECTIONS
    // Updates current analyser when analyser is selected from drop down
    connect(m_uiForm.cbAnalyser, SIGNAL(activated(int)), this, SLOT(analyserSelected(int)));
    // Updates current reflection when reflection is selected from drop down
    connect(m_uiForm.cbReflection, SIGNAL(activated(int)), this, SLOT(reflectionSelected(int)));
    // Shows required mapping option UI widgets when a new mapping option is selected from drop down
    connect(m_uiForm.cbMappingOptions, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(mappingOptionSelected(const QString&)));
    // Shows background removal dialog when user clicks Background Removal
    connect(m_uiForm.pbBack_2, SIGNAL(clicked()), this, SLOT(backgroundClicked()));
    // Plots raw input data when user clicks Plot Time
    connect(m_uiForm.pbPlotRaw, SIGNAL(clicked()), this, SLOT(plotRaw()));
    // Enables/disables rebin options when user toggles Do Not Rebin checkbox
    connect(m_uiForm.rebin_ckDNR, SIGNAL(toggled(bool)), this, SLOT(rebinEntryToggle(bool)));
    // Enables/disables detail balance option when user toggle Detailed Balance checkbox
    connect(m_uiForm.ckDetailedBalance, SIGNAL(toggled(bool)), this, SLOT(detailedBalanceCheck(bool)));
    // Enables/disables scale multiply option when user toggles Scale checkbox
    connect(m_uiForm.ckScaleMultiplier, SIGNAL(toggled(bool)), this, SLOT(scaleMultiplierCheck(bool)));
    connect(m_uiForm.ind_calibFile, SIGNAL(fileTextChanged(const QString &)), this, SLOT(calibFileChanged(const QString &)));
    // Enables/disables calibration file options when user toggles Use Calib File checkbox
    connect(m_uiForm.ckUseCalib, SIGNAL(toggled(bool)), this, SLOT(useCalib(bool)));
    // Displays correct UI widgets for selected rebin type when changed via Rebin Steps drop down
    connect(m_uiForm.comboRebinType, SIGNAL(currentIndexChanged(int)), m_uiForm.swIndRebin, SLOT(setCurrentIndex(int)));
    // Shows message on run buton when user is inputting a run number
    connect(m_uiForm.ind_runFiles, SIGNAL(fileTextChanged(const QString &)), this, SLOT(pbRunEditing()));
    // Shows message on run button when Mantid is finding the file for a given run number
    connect(m_uiForm.ind_runFiles, SIGNAL(findingFiles()), this, SLOT(pbRunFinding()));
    // Reverts run button back to normal when file finding has finished
    connect(m_uiForm.ind_runFiles, SIGNAL(fileFindingFinished()), this, SLOT(pbRunFinished()));
    // Perform validation when editing an option
    connect(m_uiForm.leDetailedBalance, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.leScaleMultiplier, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.leSpectraMin, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.leSpectraMax, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.entryRebinLow, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.entryRebinWidth, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));
    connect(m_uiForm.entryRebinHigh, SIGNAL(textChanged(const QString &)), this, SLOT(validateTab()));

    // Update UI widgets to show default values
    mappingOptionSelected(m_uiForm.cbMappingOptions->currentText());
    rebinEntryToggle(m_uiForm.rebin_ckDNR->isChecked());
    backgroundRemoval();

    // Validate to remove invalid markers
    validateTab();
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  IndirectConvertToEnergy::~IndirectConvertToEnergy()
  {
  }
  
  void IndirectConvertToEnergy::setup()
  {
  }

  void IndirectConvertToEnergy::run()
  {
    QString pyInput =
      "import inelastic_indirect_reducer as iir\n"
      "reducer = iir.IndirectReducer()\n"
      "reducer.set_instrument_name('" + m_uiForm.cbInst->currentText() + "')\n"
      "reducer.set_detector_range(" +m_uiForm.leSpectraMin->text()+ "-1, " +m_uiForm.leSpectraMax->text()+ "-1)\n"
      "reducer.set_parameter_file('" + QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory")) + m_uiForm.cbInst->currentText() + "_" + m_uiForm.cbAnalyser->currentText() + "_" + m_uiForm.cbReflection->currentText() + "_Parameters.xml')\n";

    QStringList files = m_uiForm.ind_runFiles->getFilenames();
    for ( QStringList::iterator it = files.begin(); it != files.end(); ++it )
    {
      pyInput += "reducer.append_data_file(r'" + *it + "')\n";
    }

    if ( m_uiForm.ckSumFiles->isChecked() )
    {
      pyInput += "reducer.set_sum_files(True)\n";
    }

    if ( m_bgRemoval )
    {
      QPair<double,double> background = m_backgroundDialog->getRange();
      pyInput += "reducer.set_background("+QString::number(background.first)+", "+QString::number(background.second)+")\n";
    }

    if ( m_uiForm.ckUseCalib->isChecked() )
    {
      pyInput +=
        "from IndirectCommon import loadNexus\n"
        "reducer.set_calibration_workspace(loadNexus(r'"+m_uiForm.ind_calibFile->getFirstFilename()+"'))\n";
    }

    if ( m_uiForm.ckLoadLogs->isChecked() )
    {
      pyInput += "reducer.set_load_logs(True)\n";
    }

    if ( ! m_uiForm.rebin_ckDNR->isChecked() )
    {
      QString rebin;
      if ( m_uiForm.comboRebinType->currentIndex() == 0 )
      {
        rebin = m_uiForm.entryRebinLow->text() + "," + m_uiForm.entryRebinWidth->text() + "," + m_uiForm.entryRebinHigh->text();
      }
      else
      {
        rebin = m_uiForm.entryRebinString->text();
      }
      pyInput += "reducer.set_rebin_string('"+rebin+"')\n";
    }

    if ( m_uiForm.ckDetailedBalance->isChecked() )
    {
      pyInput += "reducer.set_detailed_balance(" + m_uiForm.leDetailedBalance->text() + ")\n";
    }

    if ( m_uiForm.ckScaleMultiplier->isChecked() )
    {
      pyInput += "reducer.set_scale_factor(" + m_uiForm.leScaleMultiplier->text() + ")\n";
    }

    if ( m_uiForm.cbMappingOptions->currentText() != "Default" )
    {
      QString grouping = createMapFile(m_uiForm.cbMappingOptions->currentText());
      pyInput += "reducer.set_grouping_policy('" + grouping + "')\n";
    }

    if ( ! m_uiForm.ckRenameWorkspace->isChecked() )
    {
      pyInput += "reducer.set_rename(False)\n";
    }

    if ( ! m_uiForm.ckFold->isChecked() )
    {
      pyInput += "reducer.set_fold_multiple_frames(False)\n";
    }

    if( m_uiForm.ckCm1Units->isChecked() )
    {
      pyInput += "reducer.set_save_to_cm_1(True)\n";
    }

    pyInput += "reducer.set_save_formats([" + savePyCode() + "])\n";

    pyInput +=
      "reducer.reduce()\n"
      "ws_list = reducer.get_result_workspaces()\n";

    // Plot Output options
    switch ( m_uiForm.ind_cbPlotOutput->currentIndex() )
    {
      case 0: // "None"
        break;
      case 1: // "Spectra"
        {
          // Plot a spectra of the first result workspace
          pyInput += 
            "if ( len(ws_list) > 0 ):\n"
            "  nSpec = mtd[ws_list[0]].getNumberHistograms()\n"
            "  plotSpectrum(ws_list[0], range(0, nSpec))\n";
        }
        break;
      case 2: // "Contour"
        {
          // Plot a 2D Contour Lines of the first result workspace
          pyInput += 
            "if ( len(ws_list) > 0 ):\n"
            "  ws = importMatrixWorkspace(ws_list[0])\n"
            "  ws.plotGraph2D()\n";
        }
        break;
    }

    // add sample logs to each of the workspaces
    QString calibChecked = m_uiForm.ckUseCalib->isChecked() ? "True" : "False";
    QString detailedBalance = m_uiForm.ckDetailedBalance->isChecked() ? "True" : "False";
    QString scaled = m_uiForm.ckScaleMultiplier->isChecked() ? "True" : "False";
    pyInput += "calibCheck = "+calibChecked+"\n"
      "detailedBalance = "+detailedBalance+"\n"
      "scaled = "+scaled+"\n"
      "for ws in ws_list:\n"
      "  AddSampleLog(Workspace=ws, LogName='calib_file', LogType='String', LogText=str(calibCheck))\n"
      "  if calibCheck:\n"
      "    AddSampleLog(Workspace=ws, LogName='calib_file_name', LogType='String', LogText='"+m_uiForm.ind_calibFile->getFirstFilename()+"')\n"
      "  AddSampleLog(Workspace=ws, LogName='detailed_balance', LogType='String', LogText=str(detailedBalance))\n"
      "  if detailedBalance:\n"
      "    AddSampleLog(Workspace=ws, LogName='detailed_balance_temp', LogType='Number', LogText='"+m_uiForm.leDetailedBalance->text()+"')\n"
      "  AddSampleLog(Workspace=ws, LogName='scale', LogType='String', LogText=str(scaled))\n"
      "  if scaled:\n"
      "    AddSampleLog(Workspace=ws, LogName='scale_factor', LogType='Number', LogText='"+m_uiForm.leScaleMultiplier->text()+"')\n";

    QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();
  }

  bool IndirectConvertToEnergy::validate()
  {
    bool valid = true;

    // run files input
    if ( ! m_uiForm.ind_runFiles->isValid() )
    {
      valid = false;
    }

    // calib file input
    if ( m_uiForm.ckUseCalib->isChecked() && !m_uiForm.ind_calibFile->isValid() )
    {
      valid = false;
    }

    // mapping selection
    if (
        ( m_uiForm.cbMappingOptions->currentText() == "Groups" && m_uiForm.leNoGroups->text() == "" ) 
        ||
        ( m_uiForm.cbMappingOptions->currentText() == "File" && ! m_uiForm.ind_mapFile->isValid() )
       )
    {
      valid = false;
      m_uiForm.valNoGroups->setText("*");
    }
    else
    {
      m_uiForm.valNoGroups->setText("");
    }

    int dummyPos = 0;

    QString text = m_uiForm.leDetailedBalance->text();
    QValidator::State fieldState = m_uiForm.leDetailedBalance->validator()->validate(text, dummyPos);

    // detailed balance
    if ( m_uiForm.ckDetailedBalance->isChecked() && fieldState != QValidator::Acceptable )
    {
      valid = false;
      m_uiForm.valDetailedBalance->setText("*");
    }
    else
    {
      m_uiForm.valDetailedBalance->setText("");
    }

    int dummyPos2 = 0;

    // scale multiplier
    QString scaleMultiplierText = m_uiForm.leScaleMultiplier->text();
    QValidator::State fieldState2 = m_uiForm.leScaleMultiplier->validator()->validate(scaleMultiplierText, dummyPos2);

    if ( m_uiForm.ckScaleMultiplier->isChecked() && fieldState2 != QValidator::Acceptable )
    {
      valid = false;
      m_uiForm.valScaleMultiplier->setText("*");
    }
    else
    {
      m_uiForm.valScaleMultiplier->setText("");
    }

    // SpectraMin/SpectraMax
    const QString specMin = m_uiForm.leSpectraMin->text();
    const QString specMax = m_uiForm.leSpectraMax->text();

    if (specMin.isEmpty() || specMax.isEmpty() ||
        (specMin.toDouble() < 1 || specMax.toDouble() < 1) ||  
        (specMin.toDouble() > specMax.toDouble()))
    {
      valid = false;
      m_uiForm.valSpectraMin->setText("*");
      m_uiForm.valSpectraMax->setText("*");
    }
    else
    {
      m_uiForm.valSpectraMin->setText("");
      m_uiForm.valSpectraMax->setText("");
    }

    if ( ! m_uiForm.rebin_ckDNR->isChecked() )
    {
      if ( m_uiForm.comboRebinType->currentIndex() == 0 )
      {
        if ( m_uiForm.entryRebinLow->text() == "" )
        {
          valid = false;
          m_uiForm.valELow->setText("*");
        }
        else
        {
          m_uiForm.valELow->setText("");
        }

        if ( m_uiForm.entryRebinWidth->text() == "" )
        {
          valid = false;
          m_uiForm.valEWidth->setText("*");
        }
        else
        {
          m_uiForm.valEWidth->setText("");
        }

        if ( m_uiForm.entryRebinHigh->text() == "" )
        {
          valid = false;
          m_uiForm.valEHigh->setText("*");
        }
        else
        {
          m_uiForm.valEHigh->setText("");
        }

        if ( m_uiForm.entryRebinLow->text().toDouble() > m_uiForm.entryRebinHigh->text().toDouble() )
        {
          valid = false;
          m_uiForm.valELow->setText("*");
          m_uiForm.valEHigh->setText("*");
        }
      }
      else
      {
        if ( m_uiForm.entryRebinString->text() == "" )
        {
          valid = false;
        }
      }
    }
    else
    {
      m_uiForm.valELow->setText("");
      m_uiForm.valEWidth->setText("");
      m_uiForm.valEHigh->setText("");
    }

    return valid;
  }

  /**
   * This function is called when the user selects an analyser from the cbAnalyser QComboBox
   * object. It's main purpose is to initialise the values for the Reflection ComboBox.
   * @param index :: Index of the value selected in the combo box.
   */
  void IndirectConvertToEnergy::analyserSelected(int index)
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
   * @param index :: Index of the value selected in the combo box.
   */
  void IndirectConvertToEnergy::reflectionSelected(int index)
  {
    UNUSED_ARG(index);
    // first, clear values in assosciated boxes:
    clearReflectionInfo();

    std::map<QString, QString> instDetails = getInstrumentDetails();

    if ( instDetails.size() < 3 )
    {
      emit showMessageBox("Could not gather necessary data from parameter file.");
      return;
    }
    else
    {
      m_uiForm.leSpectraMin->setText(instDetails["SpectraMin"]);
      m_uiForm.leSpectraMax->setText(instDetails["SpectraMax"]);

      if ( instDetails.size() >= 8 )
      {
        m_uiForm.leEfixed->setText(instDetails["EFixed"]);
      }
      else
      {
        m_uiForm.leEfixed->clear();
      }

      // Default rebinning parameters can be set in instrument parameter file
      if ( instDetails.size() == 9 )
      {
        m_uiForm.entryRebinString->setText(instDetails["RebinString"]);
        m_uiForm.rebin_ckDNR->setChecked(false);
        QStringList rbp = instDetails["RebinString"].split(",", QString::SkipEmptyParts);
        if ( rbp.size() == 3 )
        {
          m_uiForm.entryRebinLow->setText(rbp[0]);
          m_uiForm.entryRebinWidth->setText(rbp[1]);
          m_uiForm.entryRebinHigh->setText(rbp[2]);
          m_uiForm.comboRebinType->setCurrentIndex(0);
        }
        else
        {
          m_uiForm.comboRebinType->setCurrentIndex(1);
        }
      }
      else
      {
        m_uiForm.rebin_ckDNR->setChecked(true);
        m_uiForm.entryRebinLow->setText("");
        m_uiForm.entryRebinWidth->setText("");
        m_uiForm.entryRebinHigh->setText("");
        m_uiForm.entryRebinString->setText("");
      }
    }
  }

  /**
   * This function runs when the user makes a selection on the cbMappingOptions QComboBox.
   * @param groupType :: Value of selection made by user.
   */
  void IndirectConvertToEnergy::mappingOptionSelected(const QString& groupType)
  {
    if ( groupType == "File" )
    {
      m_uiForm.swMapping->setCurrentIndex(0);
    }
    else if ( groupType == "Groups" )
    {
      m_uiForm.swMapping->setCurrentIndex(1);
    }
    else if ( groupType == "All" || groupType == "Individual" || groupType == "Default" )
    {
      m_uiForm.swMapping->setCurrentIndex(2);
    }
  }

  /**
   * This function is called when the user clicks on the Background Removal button. It
   * displays the Background Removal dialog, initialising it if it hasn't been already.
   */
  void IndirectConvertToEnergy::backgroundClicked()
  {
    if(!m_backgroundDialog)
    {
      m_backgroundDialog = new Background(m_parentWidget);
      connect(m_backgroundDialog, SIGNAL(accepted()), this, SLOT(backgroundRemoval()));
      connect(m_backgroundDialog, SIGNAL(rejected()), this, SLOT(backgroundRemoval()));
    }
    m_backgroundDialog->show();
  }

  /**
   * Slot called when m_backgroundDialog is closed. Assesses whether user desires background removal.
   * Can be called before m_backgroundDialog even exists, for the purposes of setting the button to
   * it's initial (default) value.
   */
  void IndirectConvertToEnergy::backgroundRemoval()
  {
    if(m_backgroundDialog != NULL)
      m_bgRemoval = m_backgroundDialog->removeBackground();

    if(m_bgRemoval)
      m_uiForm.pbBack_2->setText("Background Removal (On)");
    else
      m_uiForm.pbBack_2->setText("Background Removal (Off)");
  }

  /**
   * This function holds any steps that must be performed on the selection of an instrument,
   * for example loading values from the Instrument Definition File (IDF).
   * @param prefix :: The selected instruments prefix in Mantid.
   */
  void IndirectConvertToEnergy::setIDFValues(const QString & prefix)
  {
    UNUSED_ARG(prefix);
    // empty ComboBoxes, LineEdits,etc of previous values
    m_uiForm.cbAnalyser->clear();
    m_uiForm.cbReflection->clear();
    clearReflectionInfo();

    rebinEntryToggle(m_uiForm.rebin_ckDNR->isChecked());
    detailedBalanceCheck(m_uiForm.ckDetailedBalance->isChecked());
    /* resCheck(m_uiForm.cal_ckRES->isChecked()); */

    scaleMultiplierCheck(m_uiForm.ckScaleMultiplier->isChecked());

    // Get list of analysers and populate cbAnalyser
    QString pyInput = 
      "from IndirectEnergyConversion import getInstrumentDetails\n"
      "result = getInstrumentDetails('" + m_uiForm.cbInst->currentText() + "')\n"
      "print result\n";

    QString pyOutput = m_pythonRunner.runPythonCode(pyInput, false).trimmed();

    if ( pyOutput == "" )
    {
      emit showMessageBox("Could not get list of analysers from Instrument Parameter file.");
    }
    else
    {
      QStringList analysers = pyOutput.split("\n", QString::SkipEmptyParts);

      for (int i = 0; i< analysers.count(); i++ )
      {
        QString text; // holds Text field of combo box (name of analyser)

        if ( text != "diffraction" ) // do not put diffraction into the analyser list
        {
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
      }

      analyserSelected(m_uiForm.cbAnalyser->currentIndex());
    }
  }

  /**
   * This function clears the values of the QLineEdit objec  ts used to hold Reflection-specific
   * information.
   */
  void IndirectConvertToEnergy::clearReflectionInfo()
  {
    m_uiForm.leSpectraMin->clear();
    m_uiForm.leSpectraMax->clear();
    m_uiForm.leEfixed->clear();
  }

  /**
   * This function will disable the necessary elements of the interface when the user selects "Do Not Rebin"
   * and enable them again when this is de-selected.
   *
   * @param state :: whether the "Do Not Rebin" checkbox is checked
   */
  void IndirectConvertToEnergy::rebinEntryToggle(bool state) 
  {
    //Determine value for single rebin field
    QString val;
    if(state)
      val = " ";
    else
      val = "*";

    //Rebin mode selection
    m_uiForm.comboRebinType->setEnabled(!state);
    m_uiForm.labelRebinSteps->setEnabled(!state);

    //Single rebin text entry
    m_uiForm.labelRebinLow->setEnabled( !state );
    m_uiForm.labelRebinWidth->setEnabled( !state );
    m_uiForm.labelRebinHigh->setEnabled( !state );
    m_uiForm.entryRebinLow->setEnabled( !state );
    m_uiForm.entryRebinWidth->setEnabled( !state );
    m_uiForm.entryRebinHigh->setEnabled( !state );

    //Rebin required markers
    m_uiForm.valELow->setEnabled(!state);
    m_uiForm.valELow->setText(val);
    m_uiForm.valEWidth->setEnabled(!state);
    m_uiForm.valEWidth->setText(val);
    m_uiForm.valEHigh->setEnabled(!state);
    m_uiForm.valEHigh->setText(val);

    //Rebin string entry
    m_uiForm.entryRebinString->setEnabled(!state);
    m_uiForm.labelRebinString->setEnabled(!state);
  }

  /**
   * Disables/enables the relevant parts of the UI when user checks/unchecks the Detailed Balance
   * ckDetailedBalance checkbox.
   * @param state :: state of the checkbox
   */
  void IndirectConvertToEnergy::detailedBalanceCheck(bool state)
  {
    m_uiForm.leDetailedBalance->setEnabled(state);
    m_uiForm.lbDBKelvin->setEnabled(state);
  }

  /**
   * Disables/enables the relevant parts of the UI when user checks/unchecks the 'Multiplication Factor (Scale):'
   * ckScaleMultiplier checkbox.
   * @param state :: state of the checkbox
   */
  void IndirectConvertToEnergy::scaleMultiplierCheck(bool state)
  {
    m_uiForm.leScaleMultiplier->setEnabled(state);
  }

  /**
   * This function creates the mapping/grouping file for the data analysis.
   * @param groupType :: Type of grouping (All, Group, Indiviual)
   * @return path to mapping file, or an empty string if file could not be created.
   */
  QString IndirectConvertToEnergy::createMapFile(const QString& groupType)
  {
    QString groupFile, ngroup, nspec;
    QString ndet = "( "+m_uiForm.leSpectraMax->text()+" - "+m_uiForm.leSpectraMin->text()+") + 1";

    if ( groupType == "File" )
    {
      groupFile = m_uiForm.ind_mapFile->getFirstFilename();
      if ( groupFile == "" )
      {
        emit showMessageBox("You must enter a path to the .map file.");
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
      return "All";
    }
    else if ( groupType == "Individual" )
    {
      return "Individual";
    }

    groupFile = m_uiForm.cbInst->itemData(m_uiForm.cbInst->currentIndex()).toString().toLower();
    groupFile += "_" + m_uiForm.cbAnalyser->currentText() + m_uiForm.cbReflection->currentText();
    groupFile += "_" + groupType + ".map";	

    QString pyInput =
      "import IndirectEnergyConversion as ind\n"
      "mapfile = ind.createMappingFile('"+groupFile+"', %1, %2, %3)\n"
      "print mapfile\n";
    pyInput = pyInput.arg(ngroup);
    pyInput = pyInput.arg(nspec);
    pyInput = pyInput.arg(m_uiForm.leSpectraMin->text());

    QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();

    return pyOutput;
  }

  /**
   * This function creates the Python script necessary to set the variables used for saving data
   * in the main convert_to_energy script.
   * @return python code as a string
   */
  QString IndirectConvertToEnergy::savePyCode()
  {
    QStringList fileFormats;
    QString fileFormatList;

    if ( m_uiForm.save_ckNexus->isChecked() )
      fileFormats << "nxs";
    if ( m_uiForm.save_ckSPE->isChecked() )
      fileFormats << "spe";
    if ( m_uiForm.save_ckNxSPE->isChecked() )
      fileFormats << "nxspe";
    if ( m_uiForm.save_ckAscii->isChecked() )
      fileFormats << "ascii";
    if ( m_uiForm.save_ckAclimax->isChecked() )
      fileFormats << "aclimax";

    if ( fileFormats.size() != 0 )
      fileFormatList = "'" + fileFormats.join("', '") + "'";
    else
      fileFormatList = "";

    return fileFormatList;
  }

  /**
   * Plots raw time data from .raw file before any data conversion has been performed.
   */
  void IndirectConvertToEnergy::plotRaw()
  {
    if ( m_uiForm.ind_runFiles->isValid() )
    {
      bool ok;
      QString spectraRange = QInputDialog::getText(0, "Insert Spectra Ranges", "Range: ", QLineEdit::Normal, m_uiForm.leSpectraMin->text() +"-"+ m_uiForm.leSpectraMax->text(), &ok);

      if ( !ok || spectraRange.isEmpty() )
      {
        return;
      }
      QStringList specList = spectraRange.split("-");

      QString rawFile = m_uiForm.ind_runFiles->getFirstFilename();
      if ( (specList.size() > 2) || ( specList.size() < 1) )
      {
        emit showMessageBox("Invalid input. Must be of form <SpecMin>-<SpecMax>");
        return;
      }
      if ( specList.size() == 1 )
      {
        specList.append(specList[0]);
      }

      QString bgrange;

      if ( m_bgRemoval )
      {
        QPair<double, double> range = m_backgroundDialog->getRange();
        bgrange = "[ " + QString::number(range.first) + "," + QString::number(range.second) + " ]";
      }
      else
      {
        bgrange = "[-1, -1]";
      }

      QString pyInput =
        "from mantid.simpleapi import CalculateFlatBackground,GroupDetectors,Load\n"
        "from mantidplot import plotSpectrum\n"
        "import os.path as op\n"
        "file = r'" + rawFile + "'\n"
        "name = op.splitext( op.split(file)[1] )[0]\n"
        "bgrange = " + bgrange + "\n"
        "Load(Filename=file, OutputWorkspace=name, SpectrumMin="+specList[0]+", SpectrumMax="+specList[1]+")\n"
        "if ( bgrange != [-1, -1] ):\n"
        "    #Remove background\n"
        "    CalculateFlatBackground(InputWorkspace=name, OutputWorkspace=name+'_bg', StartX=bgrange[0], EndX=bgrange[1], Mode='Mean')\n"
        "    GroupDetectors(InputWorkspace=name+'_bg', OutputWorkspace=name+'_grp', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
        "    GroupDetectors(InputWorkspace=name, OutputWorkspace=name+'_grp_raw', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
        "else: # Just group detectors as they are\n"
        "    GroupDetectors(InputWorkspace=name, OutputWorkspace=name+'_grp', DetectorList=range("+specList[0]+","+specList[1]+"+1))\n"
        "graph = plotSpectrum(name+'_grp', 0)\n";

      QString pyOutput = m_pythonRunner.runPythonCode(pyInput).trimmed();

      if ( pyOutput != "" )
      {
        emit showMessageBox(pyOutput);
      }
    }
    else
    {
      emit showMessageBox("You must select a run file.");
    }
  }

  void IndirectConvertToEnergy::useCalib(bool state)
  {
    m_uiForm.ind_calibFile->isOptional(!state);
    m_uiForm.ind_calibFile->setEnabled(state);
  }

  /**
   * Controls the ckUseCalib checkbox to automatically check it when a user inputs a file from clicking on 'browse'.
   * @param calib :: path to calib file
   */
  void IndirectConvertToEnergy::calibFileChanged(const QString & calib)
  {
    if ( calib.isEmpty() )
    {
      m_uiForm.ckUseCalib->setChecked(false);
    }
    else
    {
      m_uiForm.ckUseCalib->setChecked(true);
    }
  }

  /**
   * Called when a user starts to type / edit the runs to load.
   */
  void IndirectConvertToEnergy::pbRunEditing()
  {
    m_uiForm.pbRun->setEnabled(false);
    m_uiForm.pbRun->setText("Editing...");
  }

  /**
   * Called when the FileFinder starts finding the files.
   */
  void IndirectConvertToEnergy::pbRunFinding()
  {
    m_uiForm.pbRun->setText("Finding files...");
    m_uiForm.ind_runFiles->setEnabled(false);
  }

  /**
   * Called when the FileFinder has finished finding the files.
   */
  void IndirectConvertToEnergy::pbRunFinished()
  {
    if(!m_uiForm.ind_runFiles->isValid())
    {
      m_uiForm.pbRun->setText("Invalid Run");
    }
    else
    {
      m_uiForm.pbRun->setText("Run");
      m_uiForm.pbRun->setEnabled(true);
    }
    m_uiForm.ind_runFiles->setEnabled(true);
  }

} // namespace CustomInterfaces
} // namespace Mantid
