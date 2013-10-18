#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtCustomInterfaces/CalcCorr.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/WorkspaceSelector.h"


#include <QLineEdit>
#include <QList>
#include <QValidator>
#include <QDoubleValidator>

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
      assert(range->first < range->second); // Play nice.

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

    // Connect up all fields to inputChanged method of IDATab (calls validate).
    foreach(QLineEdit * field, allFields)
    {
      connect(field, SIGNAL(textEdited(const QString &)), this, SLOT(inputChanged()));
    }

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

    QString width = uiForm().absp_lewidth->text();

    QString filename = uiForm().absp_dsSampleInput->getCurrentDataName();

    if ( !Mantid::API::AnalysisDataService::Instance().doesExist(filename.toStdString()) )
    {
      QString input = uiForm().absp_dsSampleInput->getFullFilePath();
      if ( input == "" ) { return; }
      pyInput +=
      "import os.path as op\n"
      "file = r'" + input + "'\n"
      "( dir, filename ) = op.split(file)\n"
      "( name, ext ) = op.splitext(filename)\n"
      "LoadNexusProcessed(Filename=file, OutputWorkspace=name)\n"
      "inputws = name\n";
    }
    else
    {
      pyInput += "inputws = '" + filename + "'\n";
    }
  
    if ( uiForm().absp_ckUseCan->isChecked() )
    {
      QString canFile = uiForm().absp_dsCanInput->getCurrentDataName();

      //load the can file / get the can workspace 
      if ( !Mantid::API::AnalysisDataService::Instance().doesExist(canFile.toStdString()) )
      {
        QString input = uiForm().absp_dsCanInput->getFullFilePath();
        if ( input == "" ) { return; }
        pyInput +=
        "import os.path as op\n"
        "file = r'" + input + "'\n"
        "( dir, filename ) = op.split(file)\n"
        "( name, ext ) = op.splitext(filename)\n"
        "LoadNexusProcessed(Filename=file, OutputWorkspace=name)\n"
        "inputws = name\n";
      }
      else
      {
        pyInput += "inputws = '" + canFile + "'\n";
      }

      pyInput +=
        "ncan = 2\n"
        "density = [" + uiForm().absp_lesamden->text() + ", " + uiForm().absp_lecanden->text() + ", " + uiForm().absp_lecanden->text() + "]\n"
        "sigs = [" + uiForm().absp_lesamsigs->text() + "," + uiForm().absp_lecansigs->text() + "," + uiForm().absp_lecansigs->text() + "]\n"
        "siga = [" + uiForm().absp_lesamsiga->text() + "," + uiForm().absp_lecansiga->text() + "," + uiForm().absp_lecansiga->text() + "]\n";
    }
    else
    {
      pyInput +=
        "ncan = 1\n"
        "density = [" + uiForm().absp_lesamden->text() + ", 0.0, 0.0 ]\n"
        "sigs = [" + uiForm().absp_lesamsigs->text() + ", 0.0, 0.0]\n"
        "siga = [" + uiForm().absp_lesamsiga->text() + ", 0.0, 0.0]\n";
    }

    if ( uiForm().absp_ckVerbose->isChecked() ) pyInput += "verbose = True\n";
    else pyInput += "verbose = False\n";

    if ( uiForm().absp_ckSave->isChecked() ) pyInput += "save = True\n";
    else pyInput += "save = False\n";

    pyInput +=
      "geom = '" + geom + "'\n"
      "beam = [3.0, 0.5*" + width + ", -0.5*" + width + ", 2.0, -2.0, 0.0, 3.0, 0.0, 3.0]\n"
      "size = " + size + "\n"
      "avar = " + uiForm().absp_leavar->text() + "\n"
      "plotOpt = '" + uiForm().absp_cbPlotOutput->currentText() + "'\n"
      "IndirectAbsCor.AbsRunFeeder(inputws, geom, beam, ncan, size, density, sigs, siga, avar, plotOpt=plotOpt, Save=save, Verbose=verbose)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
  }

  QString CalcCorr::validate()
  {
    UserInputValidator uiv;

    // Input (file or workspace)
    QString filename = uiForm().absp_dsSampleInput->getCurrentDataName();

    if(filename.isEmpty())
    {
      uiv.addErrorMessage("You must select a Sample file or workspace.");
    }

    if ( uiForm().absp_cbShape->currentText() == "Flat" )
    {
      // Flat Geometry
      uiv.checkFieldIsValid("Thickness", uiForm().absp_lets, uiForm().absp_valts);

      if ( uiForm().absp_ckUseCan->isChecked() )
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
      if ( uiForm().absp_ckUseCan->isChecked() )
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

    uiv.checkFieldIsValid("Beam Width", uiForm().absp_lewidth, uiForm().absp_valWidth);

    // Sample details
    uiv.checkFieldIsValid("Sample Number Density",           uiForm().absp_lesamden,  uiForm().absp_valSamden);

    switch(uiForm().absp_cbSampleInputType->currentIndex())
    {
      case 0:
          //using direct input
          uiv.checkFieldIsValid("Sample Scattering Cross-Section", uiForm().absp_lesamsigs, uiForm().absp_valSamsigs);
          uiv.checkFieldIsValid("Sample Absorption Cross-Section", uiForm().absp_lesamsiga, uiForm().absp_valSamsiga);
        break;
      case 1:
          //input using formula
          uiv.checkFieldIsValid("Can Cross-Section Formula", uiForm().absp_leSampleFormula, uiForm().absp_valSampleFormula);
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
            uiv.checkFieldIsValid("Can Cross-Section Formula", uiForm().absp_leCanFormula, uiForm().absp_valCanFormula);
          break;
      }
    }

    return uiv.generateErrorMessage();
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
    // Disable thickness fields/labels/asterisks.
    uiForm().absp_lbtc1->setEnabled(checked);
    uiForm().absp_lbtc2->setEnabled(checked);
    uiForm().absp_letc1->setEnabled(checked);
    uiForm().absp_letc2->setEnabled(checked);
    uiForm().absp_valtc1->setVisible(checked);
    uiForm().absp_valtc2->setVisible(checked);

    // Disable R3 field/label/asterisk.
    uiForm().absp_lbR3->setEnabled(checked);
    uiForm().absp_ler3->setEnabled(checked);
    uiForm().absp_valR3->setVisible(checked);

    // Disable "Can Details" group and asterisks.
    uiForm().absp_gbCan->setEnabled(checked);
    uiForm().absp_valCanden->setVisible(checked);
    uiForm().absp_valCansigs->setVisible(checked);
    uiForm().absp_valCansiga->setVisible(checked);

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
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
