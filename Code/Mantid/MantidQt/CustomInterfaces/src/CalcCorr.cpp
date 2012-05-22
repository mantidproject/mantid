#include "MantidQtCustomInterfaces/CalcCorr.h"

namespace MantidQt
{
namespace CustomInterfaces
{
namespace IDA
{
  CalcCorr::CalcCorr(QWidget * parent) : 
    IDATab(parent), m_dblVal(NULL) 
  {
    m_dblVal = new QDoubleValidator(this);
  }

  void CalcCorr::setup()
  {
    // set signals and slot connections for F2Py Absorption routine
    connect(uiForm().absp_cbInputType, SIGNAL(currentIndexChanged(int)), uiForm().absp_swInput, SLOT(setCurrentIndex(int)));
    connect(uiForm().absp_cbShape, SIGNAL(currentIndexChanged(int)), this, SLOT(shape(int)));
    connect(uiForm().absp_ckUseCan, SIGNAL(toggled(bool)), this, SLOT(useCanChecked(bool)));
    connect(uiForm().absp_letc1, SIGNAL(editingFinished()), this, SLOT(tcSync()));
    // apply QValidators to items.
    uiForm().absp_lewidth->setValidator(m_dblVal);
    uiForm().absp_leavar->setValidator(m_dblVal);
    // sample
    uiForm().absp_lesamden->setValidator(m_dblVal);
    uiForm().absp_lesamsigs->setValidator(m_dblVal);
    uiForm().absp_lesamsiga->setValidator(m_dblVal);
    // can
    uiForm().absp_lecanden->setValidator(m_dblVal);
    uiForm().absp_lecansigs->setValidator(m_dblVal);
    uiForm().absp_lecansiga->setValidator(m_dblVal);
    // flat shape
    uiForm().absp_lets->setValidator(m_dblVal);
    uiForm().absp_letc1->setValidator(m_dblVal);
    uiForm().absp_letc2->setValidator(m_dblVal);
    // cylinder shape
    uiForm().absp_ler1->setValidator(m_dblVal);
    uiForm().absp_ler2->setValidator(m_dblVal);
    uiForm().absp_ler3->setValidator(m_dblVal);

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

    if ( uiForm().absp_cbInputType->currentText() == "File" )
    {
      QString input = uiForm().absp_inputFile->getFirstFilename();
      if ( input == "" ) { return; }
      pyInput +=
      "import os.path as op\n"
      "file = r'" + input + "'\n"
      "( dir, filename ) = op.split(file)\n"
      "( name, ext ) = op.splitext(filename)\n"
      "LoadNexusProcessed(file, name)\n"
      "inputws = name\n";
    }
    else
    {
      pyInput += "inputws = '" + uiForm().absp_wsInput->currentText() + "'\n";
    }
  
    if ( uiForm().absp_ckUseCan->isChecked() )
    {
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

    pyInput +=
      "geom = '" + geom + "'\n"
      "beam = [3.0, 0.5*" + width + ", -0.5*" + width + ", 2.0, -2.0, 0.0, 3.0, 0.0, 3.0]\n"
      "size = " + size + "\n"
      "avar = " + uiForm().absp_leavar->text() + "\n"
      "plotOpt = '" + uiForm().absp_cbPlotOutput->currentText() + "'\n"
      "IndirectAbsCor.AbsRunFeeder(inputws, geom, beam, ncan, size, density, sigs, siga, avar, plotOpt=plotOpt)\n";

    QString pyOutput = runPythonCode(pyInput).trimmed();
  }

  QString CalcCorr::validate()
  {
    QStringList invalidInputs;
  
    // Input (file or workspace)
    if ( uiForm().absp_cbInputType->currentText() == "File" )
    {
      if ( ! uiForm().absp_inputFile->isValid() )
        invalidInputs.append("Input File");
    }
    else
    {
      if ( uiForm().absp_wsInput->currentText() == "" )
        invalidInputs.append("Input Workspace");
    }

    if ( uiForm().absp_cbShape->currentText() == "Flat" )
    {
      // Flat Geometry
      if ( uiForm().absp_lets->text() != "" )
      {
        uiForm().absp_valts->setText(" ");
      }
      else
      {
        uiForm().absp_valts->setText("*");
        invalidInputs.append("Thickness");
      }

      if ( uiForm().absp_ckUseCan->isChecked() )
      {
        if ( uiForm().absp_letc1->text() != "" )
        {
          uiForm().absp_valtc1->setText(" ");
        }
        else
        {
          uiForm().absp_valtc1->setText("*");
          invalidInputs.append("Front Thickness");
        }

        if ( uiForm().absp_letc2->text() != "" )
        {
          uiForm().absp_valtc2->setText(" ");
        }
        else
        {
          uiForm().absp_valtc2->setText("*");
          invalidInputs.append("Back Thickness");
        }
      }
    }

    if ( uiForm().absp_cbShape->currentText() == "Cylinder" )
    {
      // Cylinder geometry
      if ( uiForm().absp_ler1->text() != "" )
      {
        uiForm().absp_valR1->setText(" ");
      }
      else
      {
        uiForm().absp_valR1->setText("*");
        invalidInputs.append("Radius 1");
      }

      if ( uiForm().absp_ler2->text() != "" )
      {
        uiForm().absp_valR2->setText(" ");
      }
      else
      {
        uiForm().absp_valR2->setText("*");
        invalidInputs.append("Radius 2");
      }
    
      // R3  only relevant when using can
      if ( uiForm().absp_ckUseCan->isChecked() )
      {
        if ( uiForm().absp_ler3->text() != "" )
        {
          uiForm().absp_valR3->setText(" ");
        }
        else
        {
          uiForm().absp_valR3->setText("*");
          invalidInputs.append("Radius 3");
        }
      }
    }

    // Can angle to beam || Step size
    if ( uiForm().absp_leavar->text() != "" )
    {
      uiForm().absp_valAvar->setText(" ");
    }
    else
    {
      uiForm().absp_valAvar->setText("*");
      invalidInputs.append("Can Angle to Beam");
    }

    // Beam Width
    if ( uiForm().absp_lewidth->text() != "" )
    {
      uiForm().absp_valWidth->setText(" ");
    }
    else
    {
      uiForm().absp_valWidth->setText("*");
      invalidInputs.append("Beam Width");
    }

    // Sample details
    if ( uiForm().absp_lesamden->text() != "" )
    {
      uiForm().absp_valSamden->setText(" ");
    }
    else
    {
      uiForm().absp_valSamden->setText("*");
      invalidInputs.append("Sample Number Density");
    }

    if ( uiForm().absp_lesamsigs->text() != "" )
    {
      uiForm().absp_valSamsigs->setText(" ");
    }
    else
    {
      uiForm().absp_valSamsigs->setText("*");
      invalidInputs.append("Sample Scattering Cross-Section");
    }

    if ( uiForm().absp_lesamsiga->text() != "" )
    {
      uiForm().absp_valSamsiga->setText(" ");
    }
    else
    {
      uiForm().absp_valSamsiga->setText("*");
      invalidInputs.append("Sample Absorption Cross-Section");
    }

    // Can details (only test if "Use Can" is checked)
    if ( uiForm().absp_ckUseCan->isChecked() )
    {
      if ( uiForm().absp_lecanden->text() != "" )
      {
        uiForm().absp_valCanden->setText(" ");
      }
      else
      {
        uiForm().absp_valCanden->setText("*");
        invalidInputs.append("Can Number Density");
      }

      if ( uiForm().absp_lecansigs->text() != "" )
      {
        uiForm().absp_valCansigs->setText(" ");
      }
      else
      {
        uiForm().absp_valCansigs->setText("*");
        invalidInputs.append("Can Scattering Cross-Section");
      }

      if ( uiForm().absp_lecansiga->text() != "" )
      {
        uiForm().absp_valCansiga->setText(" ");
      }
      else
      {
        uiForm().absp_valCansiga->setText("*");
        invalidInputs.append("Can Absorption Cross-Section");
      }
    }

    QString error = "Please check the following inputs: \n" + invalidInputs.join("\n");
    return error;
  }

  void CalcCorr::loadSettings(const QSettings & settings)
  {
    uiForm().absp_inputFile->readSettings(settings.group());
  }

  void CalcCorr::shape(int index)
  {
    uiForm().absp_swShapeDetails->setCurrentIndex(index);
    // Meaning of the "avar" variable changes depending on shape selection
    if ( index == 0 ) { uiForm().absp_lbAvar->setText("Can Angle to Beam"); }
    else if ( index == 1 ) { uiForm().absp_lbAvar->setText("Step Size"); }
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
