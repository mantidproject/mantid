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
* @param userSettings :: the form that the user filled in
* @throw invalid_argument where problems with user data prevent the calculation from proceeding
*/
deltaECalc::deltaECalc(QWidget * const interface, const Ui::ConvertToEnergy &userSettings, 
                       const bool removalBg, const double TOFWinSt, const double TOFWinEnd) :
  pythonCalc(interface), m_sets(userSettings), m_bgRemove(removalBg), m_TOFWinSt(TOFWinSt), m_TOFWinEnd(TOFWinEnd), m_diagnosedWS("")
{
}

/** Adds user values from the GUI into the Python script
* @param inputFiles :: a coma separated list of data file names
* @param whiteB :: The filename of the white beam run
*/
void deltaECalc::createProcessingScript(const QStringList &runFiles, const QString &whiteBeam,
					const QStringList &absRunFiles, const QString &absWhiteBeam,
					const QString & saveName)
{ 
  QString pyCode = "import DirectEnergyConversion as direct\n";
  pyCode += QString("mono_sample = direct.DirectEnergyConversion('%1')\n").arg(m_sets.cbInst->currentText());
  //Turn off printing to stdout
  pyCode += QString("mono_sample._to_stdout = False\n");

  addAnalysisOptions(pyCode);
  addMaskingCommands(pyCode);
  // Check save formats
  QStringList fileExts;
  if( m_sets.save_ckSPE->isChecked() )
  {
    fileExts.append("'.spe'");
  }
  if( m_sets.save_ckNexus->isChecked() )
  {
    fileExts.append("'.nxs'");
  }
  if( m_sets.save_ckNxSPE->isChecked() )
  {
    fileExts.append("'.nxspe'");
  }
  pyCode += "mono_sample.save_formats = [" + fileExts.join(",") + "]\n\n";

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

  if( m_sets.ckSumSpecs->isChecked() || runFiles.size() == 1)
  {
    QString pySaveName;
    if( saveName.isEmpty() )
    {
      pySaveName = "None";
    }
    else
    {
      pySaveName = "r'" + saveName + "'";
    }
    pyCode += QString("mono_sample.convert_to_energy(%1, %2, %3, %4, %5, %6, %7, motor=%8, offset=%9)");
    pyCode = pyCode.arg(pyRunFiles, eiGuess,pyWhiteBeam,pyAbsRunFiles,absEiGuess, pyAbsWhiteBeam, pySaveName, pyMotorName, pySeOffset);
  }
  else
  {
    QString pySaveName;
    if( saveName.isEmpty() )
    {
      pySaveName = "None";
    }
    else
    {
      pySaveName = "r'" + QFileInfo(saveName).absolutePath() + "'";
    }
    pyCode += "rfiles = " + pyRunFiles + "\n";
    if( absRunFiles.isEmpty() )
    {
      pyCode +=
        "for run in rfiles:\n"
        "  mono_sample.convert_to_energy(run, %1, %2, save_path=%3, motor=%4, offset=%5)\n";
      pyCode = pyCode.arg(eiGuess, pyWhiteBeam, pySaveName, pyMotorName, pySeOffset);
    }
    else
    {
      pyCode += "abs_rfiles = " + pyAbsRunFiles + "\n";
      pyCode +=
        "for run, abs in zip(rfiles, abs_rfiles):\n"
        "  mono_sample.convert_to_energy(run, %1, %2, abs, %3, %4, save_path=%5, motor=%6, offset=%7)\n";
      pyCode = pyCode.arg(eiGuess, pyWhiteBeam, absEiGuess, pyAbsWhiteBeam, pySaveName, pyMotorName, pySeOffset);
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
  pyCode += QString("mono_sample.normalise_method = '%1'\n").arg(inputValue);

  pyCode += QString("mono_sample.background = %1\n");
  if( this->m_bgRemove )
  {
    pyCode = pyCode.arg("True");
    pyCode += QString("mono_sample.background_range = [%1, %2]\n").arg(this->m_TOFWinSt).arg(this->m_TOFWinEnd);
  }
  else
  {
    pyCode = pyCode.arg("False");
  }

  //Convert to energy
  pyCode += QString("mono_sample.fix_ei = %1\n");
  if( m_sets.ckFixEi->isChecked() )
  {
    pyCode = pyCode.arg("True");
  }
  else
  {
    pyCode = pyCode.arg("False");
  }
  pyCode += QString("mono_sample.energy_bins = '%1,%2,%3'\n").arg(m_sets.leELow->text(), m_sets.leEWidth->text(), m_sets.leEHigh->text());
  QString mapFile = m_sets.mapFile->getFirstFilename();
  if( !mapFile.isEmpty() )
  {
    pyCode += QString("mono_sample.map_file = r'%1'\n").arg(mapFile);
  }
  if( m_sets.ckRunAbsol->isChecked() )
  {
    QString absMapFile = m_sets.absMapFile->getFirstFilename();
    if ( !absMapFile.isEmpty() )
    {
        pyCode += QString("mono_sample.abs_map_file = r'%1'\n").arg(absMapFile);
    }
    // Set the mono vanadium integration range
    pyCode += QString("mono_sample.monovan_integr_range=[float(%1),float(%2)]\n");
    pyCode = pyCode.arg(m_sets.leVanELow->text(), m_sets.leVanEHigh->text());
    // Set the sample mass and rmm
    pyCode += QString("mono_sample.sample_mass = %1\n").arg(m_sets.leSamMass->text());
    pyCode += QString("mono_sample.sample_rmm = %1\n").arg(m_sets.leRMMMass->text());
    // And any changed vanadium mass
    pyCode += QString("mono_sample.van_mass = %1\n").arg(m_sets.leVanMass->text());
  }
}


void deltaECalc::addMaskingCommands(QString & analysisScript)
{
  if( m_diagnosedWS.isEmpty() )
  {
    return;
  }
  
  analysisScript += "mono_sample.spectra_masks = '" + m_diagnosedWS + "'\n";

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
