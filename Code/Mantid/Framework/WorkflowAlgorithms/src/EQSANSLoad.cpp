//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/EQSANSLoad.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/AnalysisDataService.h"
#include <MantidAPI/FileFinder.h>
#include <MantidAPI/FileProperty.h>
#include "MantidKernel/TimeSeriesProperty.h"
#include "Poco/DirectoryIterator.h"
#include "Poco/RegularExpression.h"
#include "Poco/NumberParser.h"
#include <iostream>
#include <fstream>
#include <istream>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSLoad)

/// Sets documentation strings for this algorithm
void EQSANSLoad::initDocs()
{
  this->setWikiSummary("Load EQSANS data.");
  this->setOptionalMessage("Load EQSANS data.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void EQSANSLoad::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, ".nxs"),
      "The name of the input event Nexus file to load.");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
      "Then name of the output EventWorkspace.");
  //declareProperty("OutputMessage","",Direction::Output);

}

/// Returns the value of a run property from a given workspace
/// @param inputWS :: input workspace
/// @param pname :: name of the property to retrieve
double getRunPropertyDbl(MatrixWorkspace_sptr inputWS, const std::string& pname)
{
  Mantid::Kernel::Property* prop = inputWS->run().getProperty(pname);
  Mantid::Kernel::PropertyWithValue<double>* dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<double>* >(prop);
  return *dp;
}

std::string EQSANSLoad::findConfigFile(const int& run)
{
  // Append the standard location of EQSANS config file to the data search directory list
  std::string sns_folder = "/SNS/EQSANS/shared/instrument_configuration";
  if (Poco::File(sns_folder).exists())
    Kernel::ConfigService::Instance().appendDataSearchDir(sns_folder);

  const std::vector<std::string>& searchPaths =
      Kernel::ConfigService::Instance().getDataSearchDirs();
  std::vector<std::string>::const_iterator it = searchPaths.begin();

  int max_run_number = 0;
  std::string config_file = "";
  Poco::RegularExpression re1("eqsans_configuration.[0-9]+");
  Poco::RegularExpression re2("[0-9]+");
  for (; it != searchPaths.end(); ++it)
  {
    Poco::DirectoryIterator file_it(*it);
    Poco::DirectoryIterator end;
    for (; file_it != end; ++file_it)
    {
      if (re1.match(file_it.name()))
      {
        std::string s;
        if (re2.extract(file_it.name(), s)==1)
        {
          int run_number = 0;
          Poco::NumberParser::tryParse(s, run_number);
          if (run_number > max_run_number && run_number <= run)
          {
            max_run_number = run_number;
            config_file = file_it.path().toString();
          }
        }
      }
    }
  }
  return config_file;
}

void EQSANSLoad::readRectangularMasks(const std::string& line)
{
  // Looking for rectangular mask
  // Rectangular mask         = 7, 0; 7, 255
  Poco::RegularExpression re_key("rectangular mask", Poco::RegularExpression::RE_CASELESS);
  Poco::RegularExpression re_key_alt("elliptical mask", Poco::RegularExpression::RE_CASELESS);
  Poco::RegularExpression::Match match;
  if (re_key.match(line, 0, match) || re_key_alt.match(line, 0, match))
  {
    Poco::RegularExpression re_sig("=[ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)[ ]*[ ;,][ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)");
    if (re_sig.match(line, 0, match))
    {
      Poco::RegularExpression::MatchVec posVec;
      re_sig.match(line, 0, posVec);
      if (posVec.size()==5)
      {
        for (int i=0; i<4; i++)
        {
          std::string num_str = line.substr(posVec[i+1].offset, posVec[i+1].length);
          m_mask_as_string = m_mask_as_string + " " + num_str;
        }
        m_mask_as_string += ",";
      }
    }
  }

}

void EQSANSLoad::readTOFcuts(const std::string& line)
{
  //TODO: property to tell us whether to use the config cuts
  Poco::RegularExpression re_key("tof edge discard", Poco::RegularExpression::RE_CASELESS);
  Poco::RegularExpression::Match match;
  if (re_key.match(line, 0, match))
  {
    Poco::RegularExpression re_sig("=[ ]*([0-9]+)[ ]*[ ,][ ]*([0-9]+)");
    if (re_sig.match(line, 0, match))
    {
      Poco::RegularExpression::MatchVec posVec;
      re_sig.match(line, 0, posVec);
      if (posVec.size()==3)
      {
        std::string num_str = line.substr(posVec[1].offset, posVec[1].length);
        Poco::NumberParser::tryParseFloat(num_str, m_low_TOF_cut);
        num_str = line.substr(posVec[2].offset, posVec[2].length);
        Poco::NumberParser::tryParseFloat(num_str, m_high_TOF_cut);
      }
    }
  }
}


void EQSANSLoad::readConfigFile(const std::string& filePath)
{
  // Initialize parameters
  m_mask_as_string = "";
  m_low_TOF_cut = 0;
  m_high_TOF_cut = 0;

  std::ifstream file(filePath.c_str());
  if (!file)
  {
    g_log.error() << "Unable to open file: " << filePath << std::endl;
    throw Exception::FileError("Unable to open file: " , filePath);
  }
  g_log.information() << "Using config file: " << filePath << std::endl;

  std::string line;
  while( getline(file,line) )
  {
    boost::trim(line);
    readRectangularMasks(line);
    readTOFcuts(line);
  }

  bool use_config_mask = true;
  if (use_config_mask)
  {
    dataWS->mutableRun().addProperty("rectangular_masks", m_mask_as_string, "pixels", true);
  }

  bool use_config_cutoff = true;
  if (use_config_cutoff)
  {
    dataWS->mutableRun().addProperty("low_tof_cut", m_low_TOF_cut, "microsecond", true);
    dataWS->mutableRun().addProperty("high_tof_cut", m_high_TOF_cut, "microsecond", true);
  }

}

void EQSANSLoad::exec()
{
  const std::string fileName = getPropertyValue("Filename");

  IAlgorithm_sptr loadAlg = createSubAlgorithm("LoadEventNexus", 0, 0.2);
  loadAlg->setProperty("Filename", fileName);
  loadAlg->executeAsSubAlg();
  IEventWorkspace_sptr dataWS_tmp = loadAlg->getProperty("OutputWorkspace");
  setProperty<MatrixWorkspace_sptr>("OutputWorkspace", dataWS_tmp);
  dataWS = boost::dynamic_pointer_cast<EventWorkspace>(dataWS_tmp);

  // Get the sample-detector distance
  Mantid::Kernel::Property* prop = dataWS->run().getProperty("detectorZ");
  Mantid::Kernel::TimeSeriesProperty<double>* dp = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double>* >(prop);
  const double sdd = dp->getStatistics().mean;
  dataWS->mutableRun().addProperty("sample_detector_distance", sdd, "mm", true);

  // Move the detector to its correct position
  IAlgorithm_sptr mvAlg = createSubAlgorithm("MoveInstrumentComponent", 0.2, 0.4);
  mvAlg->setProperty<MatrixWorkspace_sptr>("Workspace", dataWS);
  mvAlg->setProperty("ComponentName", "detector1");
  mvAlg->setProperty("Z", sdd/1000.0);
  mvAlg->setProperty("RelativePosition", false);
  mvAlg->executeAsSubAlg();

  // Get the run number so we can find the proper config file
  int run_number = 0;
  std::string config_file = "";
  if (dataWS->run().hasProperty("run_number"))
  {
    Mantid::Kernel::Property* prop = dataWS->run().getProperty("run_number");
    std::cout << "Reading run..." << std::endl;
    Mantid::Kernel::PropertyWithValue<std::string>* dp = dynamic_cast<Mantid::Kernel::PropertyWithValue<std::string>* >(prop);
    const std::string run_str = *dp;
    Poco::NumberParser::tryParse(run_str, run_number);
    // Find a proper config file
    config_file = findConfigFile(run_number);
  } else {
    g_log.error() << "Could not find run number for workspace " << getPropertyValue("OutputWorkspace") << std::endl;
  }

  // Process the config file
  if (config_file.size()>0)
  {
    readConfigFile(config_file);



  } else {
    g_log.error() << "Cound not find config file for workspace " << getPropertyValue("OutputWorkspace") << std::endl;
  }



}

} // namespace WorkflowAlgorithms
} // namespace Mantid

