#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtCustomInterfaces/Indirect/CalcCorr.h"
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
    m_uiForm.setupUi(parent);

    m_dblVal = new QDoubleValidator(this);
    m_posDblVal = new QDoubleValidator(this);
    m_posDblVal->setBottom(0.0);
  }

  void CalcCorr::setup()
  {
    // set signals and slot connections for F2Py Absorption routine
    connect(m_uiForm.cbShape, SIGNAL(currentIndexChanged(int)), this, SLOT(shape(int)));
    connect(m_uiForm.ckUseCan, SIGNAL(toggled(bool)), this, SLOT(useCanChecked(bool)));
    connect(m_uiForm.letc1, SIGNAL(editingFinished()), this, SLOT(tcSync()));
    connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString&)), this, SLOT(getBeamWidthFromWorkspace(const QString&)));

    // Sort the fields into various lists.

    QList<QLineEdit*> allFields;
    QList<QLineEdit*> doubleFields;
    QList<QLineEdit*> positiveDoubleFields;

    positiveDoubleFields += m_uiForm.lets;  // Thickness
    positiveDoubleFields += m_uiForm.letc1; // Front Thickness
    positiveDoubleFields += m_uiForm.letc2; // Back Thickness

    positiveDoubleFields += m_uiForm.ler1; // Radius 1
    positiveDoubleFields += m_uiForm.ler2; // Radius 2
    positiveDoubleFields += m_uiForm.ler3; // Radius 3

    positiveDoubleFields += m_uiForm.lewidth; // Beam Width

    positiveDoubleFields += m_uiForm.lesamden;  // Sample Number Density
    positiveDoubleFields += m_uiForm.lesamsigs; // Sample Scattering Cross-Section
    positiveDoubleFields += m_uiForm.lesamsiga; // Sample Absorption Cross-Section

    positiveDoubleFields += m_uiForm.lecanden;  // Can Number Density
    positiveDoubleFields += m_uiForm.lecansigs; // Can Scattering Cross-Section
    positiveDoubleFields += m_uiForm.lecansiga; // Can Absorption Cross-Section

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
    m_uiForm.leavar->setValidator(angleValidator); // Can Angle to Beam

    allFields = positiveDoubleFields;
    allFields += m_uiForm.leavar;

    QRegExp regex("[A-Za-z0-9\\-\\(\\)]*");
    QValidator *formulaValidator = new QRegExpValidator(regex, this);
    m_uiForm.leSampleFormula->setValidator(formulaValidator);
    m_uiForm.leCanFormula->setValidator(formulaValidator);

    // "Nudge" color of title of QGroupBox to change.
    useCanChecked(m_uiForm.ckUseCan->isChecked());
  }

  void CalcCorr::run()
  {
    QString pyInput = "import IndirectAbsCor\n";

    QString geom;
    QString size;

    if ( m_uiForm.cbShape->currentText() == "Flat" )
    {
      geom = "flt";
      if ( m_uiForm.ckUseCan->isChecked() )
      {
        size = "[" + m_uiForm.lets->text() + ", " +
        m_uiForm.letc1->text() + ", " +
        m_uiForm.letc2->text() + "]";
      }
      else
      {
        size = "[" + m_uiForm.lets->text() + ", 0.0, 0.0]";
      }
    }
    else if ( m_uiForm.cbShape->currentText() == "Cylinder" )
    {
      geom = "cyl";

      // R3 only populated when using can. R4 is fixed to 0.0
      if ( m_uiForm.ckUseCan->isChecked() )
      {
        size = "[" + m_uiForm.ler1->text() + ", " +
          m_uiForm.ler2->text() + ", " +
          m_uiForm.ler3->text() + ", 0.0 ]";
      }
      else
      {
        size = "[" + m_uiForm.ler1->text() + ", " +
          m_uiForm.ler2->text() + ", 0.0, 0.0 ]";
      }
    }

    //get beam width
    QString width = m_uiForm.lewidth->text();
    if (width.isEmpty()) { width = "None"; }

    //get sample workspace. Load from if needed.
    QString sampleWs = m_uiForm.dsSampleInput->getCurrentDataName();
    pyInput += "inputws = '" + sampleWs + "'\n";

    //sample absorption and scattering x sections.
    QString sampleScatteringXSec = m_uiForm.lesamsigs->text();
    QString sampleAbsorptionXSec = m_uiForm.lesamsiga->text();

    if ( sampleScatteringXSec.isEmpty() ) { sampleScatteringXSec = "0.0"; }
    if ( sampleAbsorptionXSec.isEmpty() ) { sampleAbsorptionXSec = "0.0"; }

    //can and sample formulas
    QString sampleFormula = m_uiForm.leSampleFormula->text();
    QString canFormula = m_uiForm.leCanFormula->text();

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
    if ( m_uiForm.ckUseCan->isChecked() )
    {
      //get sample workspace. Load from if needed.
      QString canWs = m_uiForm.dsCanInput->getCurrentDataName();
      pyInput += "canws = '" + canWs + "'\n";

      //can absoprtion and scattering x section.
      QString canScatteringXSec = m_uiForm.lecansigs->text();
      QString canAbsorptionXSec = m_uiForm.lecansiga->text();

      if ( canScatteringXSec.isEmpty() ) { canScatteringXSec = "0.0"; }
      if ( canAbsorptionXSec.isEmpty() ) { canAbsorptionXSec = "0.0"; }

      pyInput +=
        "ncan = 2\n"
        "density = [" + m_uiForm.lesamden->text() + ", " + m_uiForm.lecanden->text() + ", " + m_uiForm.lecanden->text() + "]\n"
        "sigs = [" + sampleScatteringXSec + "," + canScatteringXSec + "," + canScatteringXSec + "]\n"
        "siga = [" + sampleAbsorptionXSec + "," + canAbsorptionXSec + "," + canAbsorptionXSec + "]\n";
    }
    else
    {
      pyInput +=
        "ncan = 1\n"
        "density = [" + m_uiForm.lesamden->text() + ", 0.0, 0.0 ]\n"
        "sigs = [" + sampleScatteringXSec + ", 0.0, 0.0]\n"
        "siga = [" + sampleAbsorptionXSec + ", 0.0, 0.0]\n"
        "canws = None\n";
    }

    //Output options
    if ( m_uiForm.ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput +=
      "geom = '" + geom + "'\n"
      "beam = " + width + "\n"
      "size = " + size + "\n"
      "avar = " + m_uiForm.leavar->text() + "\n"
      "plotOpt = '" + m_uiForm.cbPlotOutput->currentText() + "'\n"
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
    bool useCan = m_uiForm.ckUseCan->isChecked();

    // Input files/workspaces
    uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);
    if (useCan)
    {
      uiv.checkDataSelectorIsValid("Can", m_uiForm.dsCanInput);

      QString sample = m_uiForm.dsSampleInput->getCurrentDataName();
      QString sampleType = sample.right(sample.length() - sample.lastIndexOf("_"));
      QString container = m_uiForm.dsCanInput->getCurrentDataName();
      QString containerType = container.right(container.length() - container.lastIndexOf("_"));

      g_log.debug() << "Sample type is: " << sampleType.toStdString() << std::endl;
      g_log.debug() << "Can type is: " << containerType.toStdString() << std::endl;

      if(containerType != sampleType)
      {
        uiv.addErrorMessage("Sample and can workspaces must contain the same type of data.");
      }
    }

    uiv.checkFieldIsValid("Beam Width", m_uiForm.lewidth, m_uiForm.valWidth);

    if ( m_uiForm.cbShape->currentText() == "Flat" )
    {
      // Flat Geometry
      uiv.checkFieldIsValid("Thickness", m_uiForm.lets, m_uiForm.valts);

      if ( useCan )
      {
        uiv.checkFieldIsValid("Front Thickness", m_uiForm.letc1, m_uiForm.valtc1);
        uiv.checkFieldIsValid("Back Thickness",  m_uiForm.letc2, m_uiForm.valtc2);
      }

      uiv.checkFieldIsValid("Can Angle to Beam must be in the range [-180 to -100], [-80 to 80] or [100 to 180].", m_uiForm.leavar,  m_uiForm.valAvar);
    }

    if ( m_uiForm.cbShape->currentText() == "Cylinder" )
    {
      // Cylinder geometry
      uiv.checkFieldIsValid("Radius 1", m_uiForm.ler1, m_uiForm.valR1);
      uiv.checkFieldIsValid("Radius 2", m_uiForm.ler2, m_uiForm.valR2);

      double radius1 = m_uiForm.ler1->text().toDouble();
      double radius2 = m_uiForm.ler2->text().toDouble();
      if( radius1 >= radius2 )
        uiv.addErrorMessage("Radius 1 should be less than Radius 2.");

      // R3 only relevant when using can
      if ( useCan )
      {
        uiv.checkFieldIsValid("Radius 3", m_uiForm.ler3, m_uiForm.valR3);

        double radius3 = m_uiForm.ler3->text().toDouble();
        if( radius2 >= radius3 )
          uiv.addErrorMessage("Radius 2 should be less than Radius 3.");

      }

      uiv.checkFieldIsValid("Step Size", m_uiForm.leavar,  m_uiForm.valAvar);

      double stepSize = m_uiForm.leavar->text().toDouble();
      if( stepSize >= (radius2 - radius1) )
        uiv.addErrorMessage("Step size should be less than (Radius 2 - Radius 1).");
    }

    // Sample details
    uiv.checkFieldIsValid("Sample Number Density", m_uiForm.lesamden, m_uiForm.valSamden);

    switch(m_uiForm.cbSampleInputType->currentIndex())
    {
      case 0:
          //using direct input
          uiv.checkFieldIsValid("Sample Scattering Cross-Section", m_uiForm.lesamsigs, m_uiForm.valSamsigs);
          uiv.checkFieldIsValid("Sample Absorption Cross-Section", m_uiForm.lesamsiga, m_uiForm.valSamsiga);
        break;
      case 1:
          //input using formula
          uiv.checkFieldIsValid("Sample Formula", m_uiForm.leSampleFormula, m_uiForm.valSampleFormula);
        break;
    }


    // Can details (only test if "Use Can" is checked)
    if ( m_uiForm.ckUseCan->isChecked() )
    {
      QString canFile = m_uiForm.dsCanInput->getCurrentDataName();
      if(canFile.isEmpty())
      {
        uiv.addErrorMessage("You must select a Sample file or workspace.");
      }

      uiv.checkFieldIsValid("Can Number Density",m_uiForm.lecanden,m_uiForm.valCanden);

      switch(m_uiForm.cbCanInputType->currentIndex())
      {
        case 0:
            // using direct input
            uiv.checkFieldIsValid("Can Scattering Cross-Section", m_uiForm.lecansigs, m_uiForm.valCansigs);
            uiv.checkFieldIsValid("Can Absorption Cross-Section", m_uiForm.lecansiga, m_uiForm.valCansiga);
          break;
        case 1:
            //input using formula
            uiv.checkFieldIsValid("Can Formula", m_uiForm.leCanFormula, m_uiForm.valCanFormula);
          break;
      }
    }

    QString error = uiv.generateErrorMessage();
    showMessageBox(error);

    return error.isEmpty();
  }

  void CalcCorr::loadSettings(const QSettings & settings)
  {
    m_uiForm.dsSampleInput->readSettings(settings.group());
    m_uiForm.dsCanInput->readSettings(settings.group());
  }

  void CalcCorr::shape(int index)
  {
    m_uiForm.swShapeDetails->setCurrentIndex(index);
    // Meaning of the "avar" variable changes depending on shape selection
    if ( index == 0 ) { m_uiForm.lbAvar->setText("Sample Angle:"); }
    else if ( index == 1 ) { m_uiForm.lbAvar->setText("Step Size:"); }
  }

  void CalcCorr::useCanChecked(bool checked)
  {

    // Disable "Can Details" group and asterisks.
    m_uiForm.gbCan->setEnabled(checked);
    m_uiForm.valCanden->setVisible(checked);
    m_uiForm.lbtc1->setEnabled(checked);
    m_uiForm.lbtc2->setEnabled(checked);
    m_uiForm.letc1->setEnabled(checked);
    m_uiForm.letc2->setEnabled(checked);
    m_uiForm.lbR3->setEnabled(checked);
    m_uiForm.ler3->setEnabled(checked);

    QString value;
    (checked ? value = "*" : value =  " ");

    m_uiForm.valCansigs->setText(value);
    m_uiForm.valCansiga->setText(value);
    m_uiForm.valCanFormula->setText(value);

    // Disable thickness fields/labels/asterisks.
    m_uiForm.valtc1->setText(value);
    m_uiForm.valtc2->setText(value);

    // // Disable R3 field/label/asterisk.
    m_uiForm.valR3->setText(value);

    if (checked)
    {
      UserInputValidator uiv;
      uiv.checkFieldIsValid("",m_uiForm.lecansigs, m_uiForm.valCansigs);
      uiv.checkFieldIsValid("",m_uiForm.lecansiga, m_uiForm.valCansiga);
      uiv.checkFieldIsValid("",m_uiForm.letc1, m_uiForm.valtc1);
      uiv.checkFieldIsValid("",m_uiForm.letc2, m_uiForm.valtc2);
      uiv.checkFieldIsValid("",m_uiForm.ler3, m_uiForm.valR3);
    }

    m_uiForm.dsCanInput->setEnabled(checked);

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

    m_uiForm.gbCan->setPalette(palette);
  }

  void CalcCorr::tcSync()
  {
    if ( m_uiForm.letc2->text() == "" )
    {
      QString val = m_uiForm.letc1->text();
      m_uiForm.letc2->setText(val);
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
      m_uiForm.lewidth->setText(QString::fromUtf8(beamWidth.c_str()));
    }
    else
    {
      m_uiForm.lewidth->setText("");
    }

  }
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
