#include "MantidQtCustomInterfaces/deltaECalc.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include <QDir>
#include <boost/lexical_cast.hpp>
#include <cmath>

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::Kernel;
using namespace Mantid::API;

const QString deltaECalc::tempWS = "mono_sample_temporyWS";

/** Read the data the user supplied to create Python code to do their calculation
* @param interface :: handle to the widget for the interface
* @param userSettings :: the form that the user filled in
* @param removalBg :: if true, remove background
* @param TOFWinSt :: start of TOF range for background
* @param TOFWinEnd :: end of TOF range for background
* @throw invalid_argument where problems with user data prevent the calculation from proceeding
*/
deltaECalc::deltaECalc(QWidget * const interface, const Ui::DirectConvertToEnergy &userSettings, 
                       const bool removalBg, const double TOFWinSt, const double TOFWinEnd) :
pythonCalc(interface), m_sets(userSettings), m_bgRemove(removalBg), m_TOFWinSt(TOFWinSt), m_TOFWinEnd(TOFWinEnd), m_diagnosedWS("")
{
}

/** Adds user values from the GUI into the Python script
* @param runFiles :: a comma separated list of data file names
* @param whiteBeam :: The filename of the white beam run
* @param absRunFiles :: run files for absolute normalization
* @param absWhiteBeam :: run file for absolute white beam normalization
* @param saveName :: filename for output saving
*/
void deltaECalc::createProcessingScript(const QStringList &runFiles, const QString &whiteBeam,
                                        const QStringList &absRunFiles, const QString &absWhiteBeam,
                                        const QString & saveName)
{ 
  QString pyCode = "import Direct.DirectEnergyConversion as direct\n";
  pyCode += QString("mono_sample = direct.DirectEnergyConversion('%1')\n").arg(m_sets.cbInst->currentText());
  //Turn off printing to stdout
  pyCode += QString("mono_sample.prop_man.log_to_mantid = True\n");

  addAnalysisOptions(pyCode);
  addMaskingCommands(pyCode);
  // Check save formats
  QStringList fileExts;
  if( m_sets.save_ckSPE->isChecked() )
  {
    fileExts.append("'spe'");
  }
  if( m_sets.save_ckNexus->isChecked() )
  {
    fileExts.append("'nxs'");
  }
  if( m_sets.save_ckNxSPE->isChecked() )
  {
    fileExts.append("'nxspe'");
  }

  if (fileExts.size()==0)
    pyCode += "mono_sample.prop_man.save_format = None\n";
  else
    pyCode += "mono_sample.prop_man.save_format = " + fileExts.join(",") + "\n\n";

  // Create the python variables. The strings are wrapped with r'' for slash safety
  QString pyRunFiles = createPyListAsString(runFiles);
  QString eiGuess = m_sets.leEGuess->text();
  QString pyWhiteBeam = (whiteBeam.isEmpty()) ? "None" : QString("r'" + whiteBeam + "'");
  // Absolute values
  QString pyAbsRunFiles = createPyListAsString(absRunFiles);
  QString absEiGuess = m_sets.leVanEi->text();
  QString pyAbsWhiteBeam = (absWhiteBeam.isEmpty()) ? "None" : QString("r'" + absWhiteBeam + "'");
  // SE Offset value
  QString seOffset = m_sets.seOffsetEdit->text();
  QString pySeOffset = (seOffset.isEmpty()) ? "None" : seOffset;
  // SE Motor Name
  QString motorName = m_sets.motorNameEdit->text();
  QString pyMotorName = (motorName.isEmpty()) ? "None" : QString("r'" + motorName + "'");

  QString None = "None";
  auto rebin    = None;
  auto map_file = None;
  pyCode += "mono_sample.prop_man.motor_name = " + pyMotorName + "\n";
  pyCode += "mono_sample.prop_man.motor_offset = " + pySeOffset + "\n";


  if( m_sets.ckSumSpecs->isChecked() || runFiles.size() == 1)
  {
    if  (m_sets.ckSumSpecs->isChecked() )
      pyCode += "mono_sample.prop_man.sum_runs = True\n";

    QString pySaveName;
    if( saveName.isEmpty() )
    {
      pyCode += "mono_sample.prop_man.save_file_name = None\n";

    }
    else
    {
      pyCode += "mono_sample.prop_man.save_file_name = r'"+saveName + "'\n";
    }
    pyCode += QString("mono_sample.convert_to_energy(%1, %2, %3, %4, %5, %6, %7)");
    pyCode = pyCode.arg(pyWhiteBeam,pyRunFiles, eiGuess,rebin,map_file,pyAbsRunFiles, pyAbsWhiteBeam);
  }
  else
  {
    QString pySaveName;
    if( saveName.isEmpty() )
    {
      pySaveName = "r'" + QFileInfo(saveName).absolutePath() + "'";
      pyCode += "mono_sample.prop_man.save_file = "+pySaveName + "\n";
    }
    pyCode += "rfiles = " + pyRunFiles + "\n";
    if( absRunFiles.isEmpty() )
    {
      pyCode +=
        "for run in rfiles:\n"
        "  mono_sample.convert_to_energy(run, %1, %2)\n";
      pyCode = pyCode.arg(eiGuess, pyWhiteBeam);
    }
    else
    {
      pyCode += "abs_rfiles = " + pyAbsRunFiles + "\n";
      pyCode +=
        "for run, abs in zip(rfiles, abs_rfiles):\n"
        "  mono_sample.convert_to_energy(%1, run, %2, %3,abs, %6)\n";
      pyCode = pyCode.arg(pyWhiteBeam,eiGuess,rebin,map_file,pyAbsRunFiles, pyAbsWhiteBeam);
      //                         pyWhiteBeam,pyRunFiles, eiGuess,rebin,map_file,pyAbsRunFiles, pyAbsWhiteBeam
    }
  }
  m_pyScript = pyCode;
}

/**
* Add the analysis options from the form to the script
* @param pyCode :: The string containing the script to update
*/
void deltaECalc::addAnalysisOptions(QString & pyCode)
{
  //Analysis options
  QString inputValue = m_sets.cbNormal->currentText();  ;
  pyCode += QString("mono_sample.prop_man.normalise_method = '%1'\n").arg(inputValue);

  pyCode += QString("mono_sample.prop_man.background = %1\n");
  if( this->m_bgRemove )
  {
    pyCode = pyCode.arg("True");
    pyCode += QString("mono_sample.prop_man.background_range = [%1, %2]\n").arg(this->m_TOFWinSt).arg(this->m_TOFWinEnd);
  }
  else
  {
    pyCode = pyCode.arg("False");
  }

  //Convert to energy
  pyCode += QString("mono_sample.prop_man.fix_ei = %1\n");
  if( m_sets.ckFixEi->isChecked() )
  {
    pyCode = pyCode.arg("True");
  }
  else
  {
    pyCode = pyCode.arg("False");
  }
  pyCode += QString("mono_sample.prop_man.energy_bins = '%1,%2,%3'\n").arg(m_sets.leELow->text(), m_sets.leEWidth->text(), m_sets.leEHigh->text());
  QString mapFile = m_sets.mapFile->getFirstFilename();
  if( !mapFile.isEmpty() )
  {
    pyCode += QString("mono_sample.prop_man.map_file = r'%1'\n").arg(mapFile);
  }
  if( m_sets.ckRunAbsol->isChecked() )
  {
    QString absMapFile = m_sets.absMapFile->getFirstFilename();
    if ( !absMapFile.isEmpty() )
    {
      pyCode += QString("mono_sample.prop_man.monovan_mapfile = r'%1'\n").arg(absMapFile);
    }
    // Set the mono vanadium integration range
    pyCode += QString("mono_sample.prop_man.monovan_integr_range=[float(%1),float(%2)]\n");
    pyCode = pyCode.arg(m_sets.leVanELow->text(), m_sets.leVanEHigh->text());
    // Set the sample mass and rmm
    pyCode += QString("mono_sample.prop_man.sample_mass = %1\n").arg(m_sets.leSamMass->text());
    pyCode += QString("mono_sample.prop_man.sample_rmm = %1\n").arg(m_sets.leRMMMass->text());
    // And any changed vanadium mass
    pyCode += QString("mono_sample.prop_man.van_mass = %1\n").arg(m_sets.leVanMass->text());
  }
}


void deltaECalc::addMaskingCommands(QString & analysisScript)
{
  if (!m_sets.ckRunAbsol->isChecked())
  {
    analysisScript += "mono_sample.prop_man.run_diagnostics = False\n";
  }

  if( m_diagnosedWS.isEmpty() )
  {
    return;
  }
  // provide pre-calculated masks
  analysisScript += "mono_sample.spectra_masks = '" + m_diagnosedWS + "'\n";
  // disable internal convert_to_energy diagnostics. We already have diagnostics workspace
  analysisScript += "mono_sample.prop_man.run_diagnostics = False\n";

  //   QString tmpWS = QString("tmp_") + m_diagnosedWS;

  //   analysisScript += "fdol_alg = FindDetectorsOutsideLimits(InputWorkspace='%1',OutputWorkspace='%2',HighThreshold=10,LowThreshold=-1,OutputFile='')\n";
  //   analysisScript += "mono_sample.spectra_masks = fdol_alg.getPropertyValue('BadSpectraNums')\n";
  //   analysisScript += "mtd.deleteWorkspace('%2')\n";

  //   analysisScript = analysisScript.arg(m_diagnosedWS).arg(tmpWS);
}

QString deltaECalc::createPyListAsString(const QStringList & names) const
{
  if( names.isEmpty() )
  {
    return "None";
  }
  QString asString = "[r'";
  asString += names.join("',r'");
  asString += "']";
  return asString;
}

/** Use the detector masking present in the workspace whose name was passed in
*  the input workspace/s
*  @param maskWS :: name of the workspace whose detector masking will be copied
*/
void deltaECalc::setDiagnosedWorkspaceName(const QString &maskWS)
{
  m_diagnosedWS = maskWS;
}
/** Insert the number before the dot of the extension
* @param filename :: the a full path or partial path or just the name of a file
* @param number :: the number to insert
* @return the filename with the number inserted
*/
std::string deltaECalc::insertNumber(const std::string &filename, const int number) const
{
  // we're going to break up the file name to insert a number into it
  Poco::Path f(filename);
  // check if the path is given in the filename
  if ( f.depth() > 0 )
  {// get the directory name, the full path of the file minus its name and add it back to the result so that we don't lose the path
    return f.directory(f.depth()-1)+"/"+f.getBaseName()+"_"+
      boost::lexical_cast<std::string>(number)+"."+f.getExtension();
  }
  return f.getBaseName()+"_"+boost::lexical_cast<std::string>(number)+
    "."+f.getExtension();
}
