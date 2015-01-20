#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtCustomInterfaces/CalcCorr.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/WorkspaceSelector.h"


#include <QLineEdit>
#include <QList>
#include <QValidator>
#include <QDoubleValidator>
#include <QRegExpValidator>

class QDoubleMultiRangeValidator : public QValidator
{
public:
  /**
   * Constructor.
   *
   * @param ranges :: a set of pairs of doubles representing the valid ranges of the input
   * @param parent :: the parent QObject of this QObject.
   */
  QDoubleMultiRangeValidator(std::set<std::pair<double, double>> ranges, QObject * parent) :
    QValidator(parent), m_ranges(ranges), m_slaveVal(NULL)
  {
    m_slaveVal = new QDoubleValidator(this);
  }

  ~QDoubleMultiRangeValidator() {}

  /**
   * Reimplemented from QValidator::validate().
   *
   * Returns Acceptable if the string input contains a double that is within at least one
   * of the ranges and is in the correct format.
   *
   * Else returns Intermediate if input contains a double that is outside the ranges or is in 
   * the wrong format; e.g. with too many digits after the decimal point or is empty.
   *
   * Else returns Invalid - i.e. the input is not a double.
   *
   * @param input :: the input string to validate
   * @param pos   :: not used.
   */
  virtual QValidator::State	validate( QString & input, int & pos ) const
  {
    UNUSED_ARG(pos);
    
    if( m_ranges.empty() )
      return Intermediate;
    
    bool acceptable = false;
    bool intermediate = false;

    // For each range in the list, use the slave QDoubleValidator to find out the state.
    for( auto range = m_ranges.begin(); range != m_ranges.end(); ++ range )
    {
      if(range->first >= range->second)
        throw std::runtime_error("Invalid range");

      m_slaveVal->setBottom(range->first);
      m_slaveVal->setTop(range->second);

      QValidator::State rangeState = m_slaveVal->validate(input, pos);

      if( rangeState == Acceptable )
        acceptable = true;
      else if( rangeState == Intermediate )
        intermediate = true;
    }

    if( acceptable )
      return Acceptable;
    if( intermediate )
      return Intermediate;

    return Invalid;
  }

private:
  /// Disallow default constructor.
  QDoubleMultiRangeValidator();

  std::set<std::pair<double, double>> m_ranges;
  QDoubleValidator * m_slaveVal;
};

namespace
{
  Mantid::Kernel::Logger g_log("CalcCorr");
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  CalcCorr::CalcCorr(QWidget * parent) : 
    IDATab(parent), m_dblVal(NULL), m_posDblVal(NULL) 
  {
    m_dblVal = new QDoubleValidator(this);
    m_posDblVal = new QDoubleValidator(this);
    m_posDblVal->setBottom(0.0);
  }

  void CalcCorr::setup()
  {
    // set signals and slot connections for F2Py Absorption routine
    connect(uiForm().absp_cbShape, SIGNAL(currentIndexChanged(int)), this, SLOT(shape(int)));
    connect(uiForm().absp_ckUseCan, SIGNAL(toggled(bool)), this, SLOT(useCanChecked(bool)));
    connect(uiForm().absp_letc1, SIGNAL(editingFinished()), this, SLOT(tcSync()));
    connect(uiForm().absp_cbSampleInputType, SIGNAL(currentIndexChanged(int)), uiForm().absp_swSampleInputType, SLOT(setCurrentIndex(int)));
    connect(uiForm().absp_cbCanInputType, SIGNAL(currentIndexChanged(int)), uiForm().absp_swCanInputType, SLOT(setCurrentIndex(int)));
    connect(uiForm().absp_dsSampleInput, SIGNAL(dataReady(const QString&)), this, SLOT(getBeamWidthFromWorkspace(const QString&)));

    // Sort the fields into various lists.

    QList<QLineEdit*> allFields;
    QList<QLineEdit*> doubleFields;
    QList<QLineEdit*> positiveDoubleFields;

    positiveDoubleFields += uiForm().absp_lets;  // Thickness
    positiveDoubleFields += uiForm().absp_letc1; // Front Thickness
    positiveDoubleFields += uiForm().absp_letc2; // Back Thickness

    positiveDoubleFields += uiForm().absp_ler1; // Radius 1
    positiveDoubleFields += uiForm().absp_ler2; // Radius 2
    positiveDoubleFields += uiForm().absp_ler3; // Radius 3
    
    positiveDoubleFields += uiForm().absp_lewidth; // Beam Width
    
    positiveDoubleFields += uiForm().absp_lesamden;  // Sample Number Density
    positiveDoubleFields += uiForm().absp_lesamsigs; // Sample Scattering Cross-Section
    positiveDoubleFields += uiForm().absp_lesamsiga; // Sample Absorption Cross-Section
    
    positiveDoubleFields += uiForm().absp_lecanden;  // Can Number Density
    positiveDoubleFields += uiForm().absp_lecansigs; // Can Scattering Cross-Section
    positiveDoubleFields += uiForm().absp_lecansiga; // Can Absorption Cross-Section

    // Set appropriate validators.
    foreach(QLineEdit * positiveDoubleField, positiveDoubleFields)
    {
      positiveDoubleField->setValidator(m_posDblVal);
    }

    // Deal with the slightly more complex multi-range "Can Angle to Beam" field.
    std::set<std::pair<double, double>> angleRanges;
    angleRanges.insert(std::make_pair(-180, -100));
    angleRanges.insert(std::make_pair(-80, 80));
    angleRanges.insert(std::make_pair(100, 180));
    QDoubleMultiRangeValidator * angleValidator = new QDoubleMultiRangeValidator(angleRanges, this);
    uiForm().absp_leavar->setValidator(angleValidator); // Can Angle to Beam

    allFields = positiveDoubleFields;
    allFields += uiForm().absp_leavar;

    QRegExp regex("[A-Za-z0-9\\-\\(\\)]*");
    QValidator *formulaValidator = new QRegExpValidator(regex, this);
    uiForm().absp_leSampleFormula->setValidator(formulaValidator);
    uiForm().absp_leCanFormula->setValidator(formulaValidator);

    // "Nudge" color of title of QGroupBox to change.
    useCanChecked(uiForm().absp_ckUseCan->isChecked());
  }

  void CalcCorr::run()
  {
    QString pyInput = "import IndirectAbsCor\n";
  
    QString geom;
    QString size;

    if ( uiForm().absp_cbShape->currentText() == "Flat" )
    {
      geom = "flt";
      if ( uiForm().absp_ckUseCan->isChecked() ) 
      {
        size = "[" + uiForm().absp_lets->text() + ", " +
        uiForm().absp_letc1->text() + ", " +
        uiForm().absp_letc2->text() + "]";
      }
      else
      {
        size = "[" + uiForm().absp_lets->text() + ", 0.0, 0.0]";
      }
    }
    else if ( uiForm().absp_cbShape->currentText() == "Cylinder" )
    {
      geom = "cyl";

      // R3 only populated when using can. R4 is fixed to 0.0
      if ( uiForm().absp_ckUseCan->isChecked() ) 
      {
        size = "[" + uiForm().absp_ler1->text() + ", " +
          uiForm().absp_ler2->text() + ", " +
          uiForm().absp_ler3->text() + ", 0.0 ]";
      }
      else
      {
        size = "[" + uiForm().absp_ler1->text() + ", " +
          uiForm().absp_ler2->text() + ", 0.0, 0.0 ]";
      }
    }

    //get beam width
    QString width = uiForm().absp_lewidth->text();
    if (width.isEmpty()) { width = "None"; }

    //get sample workspace. Load from if needed.
    QString sampleWs = uiForm().absp_dsSampleInput->getCurrentDataName();
    pyInput += "inputws = '" + sampleWs + "'\n";

    //sample absorption and scattering x sections.
    QString sampleScatteringXSec = uiForm().absp_lesamsigs->text();
    QString sampleAbsorptionXSec = uiForm().absp_lesamsiga->text();

    if ( sampleScatteringXSec.isEmpty() ) { sampleScatteringXSec = "0.0"; }
    if ( sampleAbsorptionXSec.isEmpty() ) { sampleAbsorptionXSec = "0.0"; }

    //can and sample formulas
    QString sampleFormula = uiForm().absp_leSampleFormula->text();
    QString canFormula = uiForm().absp_leCanFormula->text();

    if ( sampleFormula.isEmpty() ) 
    { 
      sampleFormula = "None";
    }
    else 
    {
      sampleFormula = "'" + sampleFormula + "'";
    }

    if ( canFormula.isEmpty() ) 
    { 
      canFormula = "None";
    }
    else 
    {
      canFormula = "'" + canFormula + "'";
    }

    //create python string to execute
    if ( uiForm().absp_ckUseCan->isChecked() )
    {
      //get sample workspace. Load from if needed.
      QString canWs = uiForm().absp_dsCanInput->getCurrentDataName();
      pyInput += "canws = '" + canWs + "'\n";

      //can absoprtion and scattering x section.
      QString canScatteringXSec = uiForm().absp_lecansigs->text();
      QString canAbsorptionXSec = uiForm().absp_lecansiga->text();

      if ( canScatteringXSec.isEmpty() ) { canScatteringXSec = "0.0"; }
      if ( canAbsorptionXSec.isEmpty() ) { canAbsorptionXSec = "0.0"; }

      pyInput +=
        "ncan = 2\n"
        "density = [" + uiForm().absp_lesamden->text() + ", " + uiForm().absp_lecanden->text() + ", " + uiForm().absp_lecanden->text() + "]\n"
        "sigs = [" + sampleScatteringXSec + "," + canScatteringXSec + "," + canScatteringXSec + "]\n"
        "siga = [" + sampleAbsorptionXSec + "," + canAbsorptionXSec + "," + canAbsorptionXSec + "]\n";
    }
    else
    {
      pyInput +=
        "ncan = 1\n"
        "density = [" + uiForm().absp_lesamden->text() + ", 0.0, 0.0 ]\n"
        "sigs = [" + sampleScatteringXSec + ", 0.0, 0.0]\n"
        "siga = [" + sampleAbsorptionXSec + ", 0.0, 0.0]\n"
        "canws = None\n";
    }

    if ( uiForm().absp_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput +=
      "geom = '" + geom + "'\n"
      "beam = " + width + "\n"
      "size = " + size + "\n"
      "avar = " + uiForm().absp_leavar->text() + "\n"
      "plotOpt = '" + uiForm().absp_cbPlotOutput->currentText() + "'\n"
      "sampleFormula = " + sampleFormula + "\n"
      "canFormula = " + canFormula + "\n"
      "print IndirectAbsCor.AbsRunFeeder(inputws, canws, geom, ncan, size, avar, density, beam, sampleFormula, canFormula, sigs, siga, plot_opt=plotOpt, save=save)\n";

    QString pyOutput = runPythonCode(pyInput);

    // Set the result workspace for Python script export
    m_pythonExportWsName = pyOutput.trimmed().toStdString();
  }

  bool CalcCorr::validate()
  {
    UserInputValidator uiv;
    bool useCan = uiForm().absp_ckUseCan->isChecked();

    // Input files/workspaces
    uiv.checkDataSelectorIsValid("Sample", uiForm().absp_dsSampleInput);
    if (useCan)
    {
      uiv.checkDataSelectorIsValid("Can", uiForm().absp_dsCanInput);

      QString sample = uiForm().absp_dsSampleInput->getCurrentDataName();
      QString sampleType = sample.right(sample.length() - sample.lastIndexOf("_"));
      QString container = uiForm().absp_dsCanInput->getCurrentDataName();
      QString containerType = container.right(container.length() - container.lastIndexOf("_"));

      g_log.debug() << "Sample type is: " << sampleType.toStdString() << std::endl;
      g_log.debug() << "Can type is: " << containerType.toStdString() << std::endl;

      if(containerType != sampleType)
      {
        uiv.addErrorMessage("Sample and can workspaces must contain the same type of data.");
      }
    }

    uiv.checkFieldIsValid("Beam Width", uiForm().absp_lewidth, uiForm().absp_valWidth);

    if ( uiForm().absp_cbShape->currentText() == "Flat" )
    {
      // Flat Geometry
      uiv.checkFieldIsValid("Thickness", uiForm().absp_lets, uiForm().absp_valts);

      if ( useCan )
      {
        uiv.checkFieldIsValid("Front Thickness", uiForm().absp_letc1, uiForm().absp_valtc1);
        uiv.checkFieldIsValid("Back Thickness",  uiForm().absp_letc2, uiForm().absp_valtc2);
      }

      uiv.checkFieldIsValid("Can Angle to Beam must be in the range [-180 to -100], [-80 to 80] or [100 to 180].", uiForm().absp_leavar,  uiForm().absp_valAvar);
    }

    if ( uiForm().absp_cbShape->currentText() == "Cylinder" )
    {
      // Cylinder geometry
      uiv.checkFieldIsValid("Radius 1", uiForm().absp_ler1, uiForm().absp_valR1);
      uiv.checkFieldIsValid("Radius 2", uiForm().absp_ler2, uiForm().absp_valR2);
      
      double radius1 = uiForm().absp_ler1->text().toDouble();
      double radius2 = uiForm().absp_ler2->text().toDouble();
      if( radius1 >= radius2 )
        uiv.addErrorMessage("Radius 1 should be less than Radius 2.");

      // R3 only relevant when using can
      if ( useCan )
      {
        uiv.checkFieldIsValid("Radius 3", uiForm().absp_ler3, uiForm().absp_valR3);
        
        double radius3 = uiForm().absp_ler3->text().toDouble();
        if( radius2 >= radius3 )
          uiv.addErrorMessage("Radius 2 should be less than Radius 3.");

      }

      uiv.checkFieldIsValid("Step Size", uiForm().absp_leavar,  uiForm().absp_valAvar);

      double stepSize = uiForm().absp_leavar->text().toDouble();
      if( stepSize >= (radius2 - radius1) )
        uiv.addErrorMessage("Step size should be less than (Radius 2 - Radius 1).");
    }

    // Sample details
    uiv.checkFieldIsValid("Sample Number Density", uiForm().absp_lesamden, uiForm().absp_valSamden);

    switch(uiForm().absp_cbSampleInputType->currentIndex())
    {
      case 0:
          //using direct input
          uiv.checkFieldIsValid("Sample Scattering Cross-Section", uiForm().absp_lesamsigs, uiForm().absp_valSamsigs);
          uiv.checkFieldIsValid("Sample Absorption Cross-Section", uiForm().absp_lesamsiga, uiForm().absp_valSamsiga);
        break;
      case 1:
          //input using formula
          uiv.checkFieldIsValid("Sample Formula", uiForm().absp_leSampleFormula, uiForm().absp_valSampleFormula);
        break;
    }


    // Can details (only test if "Use Can" is checked)
    if ( uiForm().absp_ckUseCan->isChecked() )
    {
      QString canFile = uiForm().absp_dsCanInput->getCurrentDataName();
      if(canFile.isEmpty())
      {
        uiv.addErrorMessage("You must select a Sample file or workspace.");
      }

      uiv.checkFieldIsValid("Can Number Density",uiForm().absp_lecanden,uiForm().absp_valCanden);

      switch(uiForm().absp_cbCanInputType->currentIndex())
      {
        case 0:
            // using direct input
            uiv.checkFieldIsValid("Can Scattering Cross-Section", uiForm().absp_lecansigs, uiForm().absp_valCansigs);
            uiv.checkFieldIsValid("Can Absorption Cross-Section", uiForm().absp_lecansiga, uiForm().absp_valCansiga);
          break;
        case 1:
            //input using formula
            uiv.checkFieldIsValid("Can Formula", uiForm().absp_leCanFormula, uiForm().absp_valCanFormula);
          break;
      }
    }

    QString error = uiv.generateErrorMessage();
    showMessageBox(error);

    return error.isEmpty();
  }

  void CalcCorr::loadSettings(const QSettings & settings)
  {
    uiForm().absp_dsSampleInput->readSettings(settings.group());
    uiForm().absp_dsCanInput->readSettings(settings.group());
  }

  void CalcCorr::shape(int index)
  {
    uiForm().absp_swShapeDetails->setCurrentIndex(index);
    // Meaning of the "avar" variable changes depending on shape selection
    if ( index == 0 ) { uiForm().absp_lbAvar->setText("Sample Angle:"); }
    else if ( index == 1 ) { uiForm().absp_lbAvar->setText("Step Size:"); }
  }

  void CalcCorr::useCanChecked(bool checked)
  {

    // Disable "Can Details" group and asterisks.
    uiForm().absp_gbCan->setEnabled(checked);
    uiForm().absp_valCanden->setVisible(checked);
    uiForm().absp_lbtc1->setEnabled(checked);
    uiForm().absp_lbtc2->setEnabled(checked);
    uiForm().absp_letc1->setEnabled(checked);
    uiForm().absp_letc2->setEnabled(checked);
    uiForm().absp_lbR3->setEnabled(checked);
    uiForm().absp_ler3->setEnabled(checked);
    
    QString value;
    (checked ? value = "*" : value =  " ");

    uiForm().absp_valCansigs->setText(value);
    uiForm().absp_valCansiga->setText(value);
    uiForm().absp_valCanFormula->setText(value);

    // Disable thickness fields/labels/asterisks.
    uiForm().absp_valtc1->setText(value);
    uiForm().absp_valtc2->setText(value);

    // // Disable R3 field/label/asterisk.
    uiForm().absp_valR3->setText(value);
    
    if (checked)
    {    
      UserInputValidator uiv;
      uiv.checkFieldIsValid("",uiForm().absp_lecansigs, uiForm().absp_valCansigs);
      uiv.checkFieldIsValid("",uiForm().absp_lecansiga, uiForm().absp_valCansiga);
      uiv.checkFieldIsValid("",uiForm().absp_letc1, uiForm().absp_valtc1);
      uiv.checkFieldIsValid("",uiForm().absp_letc2, uiForm().absp_valtc2);
      uiv.checkFieldIsValid("",uiForm().absp_ler3, uiForm().absp_valR3);
    }

    uiForm().absp_dsCanInput->setEnabled(checked);
  
    // Workaround for "disabling" title of the QGroupBox.
    QPalette palette;
    if(checked)
      palette.setColor(
        QPalette::Disabled, 
        QPalette::WindowText,
        QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
    else
      palette.setColor(
        QPalette::Active, 
        QPalette::WindowText,
        QApplication::palette().color(QPalette::Active, QPalette::WindowText));

    uiForm().absp_gbCan->setPalette(palette);
  }

  void CalcCorr::tcSync()
  {
    if ( uiForm().absp_letc2->text() == "" )
    {
      QString val = uiForm().absp_letc1->text();
      uiForm().absp_letc2->setText(val);
    }
  }

  void CalcCorr::getBeamWidthFromWorkspace(const QString& wsname)
  {
    using namespace Mantid::API;
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname.toStdString());
    
    if (!ws)
    {
      showMessageBox("Failed to find workspace " + wsname);
      return; 
    }

    std::string paramName = "Workflow.beam-width";
    auto instrument = ws->getInstrument();
    if (instrument->hasParameter(paramName))
    {
      std::string beamWidth = instrument->getStringParameter(paramName)[0];
      uiForm().absp_lewidth->setText(QString::fromUtf8(beamWidth.c_str()));
    }
    else
    {
      uiForm().absp_lewidth->setText("");
    }

  }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
