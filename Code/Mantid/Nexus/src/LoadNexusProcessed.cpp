// LoadNexusProcessed
// @author Ronald Fowler, based on SaveNexus
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadNexusProcessed.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/System.h"
#include "MantidNexus/NexusFileIO.h"

#include "Poco/Path.h"
#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace NeXus
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadNexusProcessed)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Initialise logger
Logger& LoadNexusProcessed::g_log = Logger::get("LoadNexusProcessed");

/// Default constructor
LoadNexusProcessed::LoadNexusProcessed() :
  Algorithm(), m_filename(), m_entrynumber(0), m_list(false), m_interval(false), m_spec_list(),
      m_spec_min(0), m_spec_max(0)
{
  nexusFile = new NexusFileIO();
}
/// Delete NexusFileIO in destructor
LoadNexusProcessed::~LoadNexusProcessed()
{
  delete nexusFile;
}

/** Initialisation method.
 *
 */
void LoadNexusProcessed::init()
{
  // Declare required input parameters for algorithm
  std::vector<std::string> exts;
  exts.push_back("NXS");
  exts.push_back("nxs");
  exts.push_back("nx5");
  exts.push_back("NX5");
  exts.push_back("xml");
  exts.push_back("XML");
  // required
  declareProperty("FileName", "", new FileValidator(exts));
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D> ("OutputWorkspace", "",
      Direction::Output));
  // optional
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int> ();
  mustBePositive->setLower(0);
  declareProperty("spectrum_min", 0, mustBePositive);
  declareProperty("spectrum_max", 0, mustBePositive->clone());
  declareProperty("EntryNumber", 0, mustBePositive->clone());
  declareProperty(new ArrayProperty<int> ("spectrum_list"));
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void LoadNexusProcessed::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("FileName");
  // Need to extract the user-defined output workspace name
  Property *ws = getProperty("OutputWorkspace");
  std::string localWSName = ws->value();
  boost::shared_ptr<Sample> sample;
  //
  m_entrynumber = getProperty("EntryNumber");

  if (nexusFile->openNexusRead(m_filename, m_entrynumber) != 0)
  {
    g_log.error("Failed to read file " + m_filename);
    throw Exception::FileError("Failed to read to file", m_filename);
  }
  if (nexusFile->getWorkspaceSize(m_numberofspectra, m_numberofchannels, m_xpoints, m_uniformbounds,
      m_axes, m_yunits) != 0)
  {
    g_log.error("Failed to read data size");
    throw Exception::FileError("Failed to read data size", m_filename);
  }

  // validate the optional parameters, if set
  checkOptionalProperties();
  int total_specs;

  // Create the 2D workspace for the output
  if (m_interval || m_list)
  {
    total_specs = m_spec_list.size();
    if (m_interval)
    {
      total_specs += (m_spec_max - m_spec_min + 1);
      m_spec_max += 1;
    }
  }
  else
  {
    total_specs = m_numberofspectra;
    m_spec_min = 1;
    m_spec_max = m_numberofspectra + 1;
  }

  // create output workspace of required size
  DataObjects::Workspace2D_sptr localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
      WorkspaceFactory::Instance().create("Workspace2D", total_specs, m_xpoints, m_numberofchannels));
  // set first axis name
  size_t colon = m_axes.find(":");
  if (colon != std::string::npos)
  {
    try
    {
      localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(m_axes.substr(0, colon));
    } catch (std::runtime_error& err)
    {
      g_log.warning("Unable to set Axis(0) units");
    }
  }
  // set Yunits
  if (m_yunits.size() > 0)
    localWorkspace->setYUnit(m_yunits);

  Histogram1D::RCtype xValues;
  xValues.access() = localWorkspace->dataX(0);
  if (m_uniformbounds)
    nexusFile->getXValues(xValues.access(), 0);
  int counter = 0;
  for (int i = 1; i <= m_numberofspectra; ++i)
  {
    //int histToRead = i + period*(m_numberOfSpectra+1);
    if ((i >= m_spec_min && i < m_spec_max) || (m_list
        && find(m_spec_list.begin(), m_spec_list.end(), i) != m_spec_list.end()))
    {
      MantidVec& values = localWorkspace->dataY(counter);
      MantidVec& errors = localWorkspace->dataE(counter);
      nexusFile->getSpectra(values, errors, i);
      if (!m_uniformbounds)
      {
        nexusFile->getXValues(xValues.access(), i - 1);
      }
      localWorkspace->setX(counter,xValues);
      localWorkspace->getAxis(1)->spectraNo(counter) = i;
      //
      ++counter;
      //if (++histCurrent % 100 == 0) progress(double(histCurrent)/total_specs); dont understand setting of histCurrent
      interruption_point();
    }
  }

  sample = localWorkspace->getSample();
  nexusFile->readNexusProcessedSample(sample);
  // Run the LoadIntsturment algorithm if name available
  if (nexusFile->readNexusInstrumentXmlName(m_instrumentxml, m_instrumentdate, m_instrumentversion))
  {
    if (m_instrumentxml != "NoXmlFileFound" && m_instrumentxml != "NoNameAvailable")
      runLoadInstrument(localWorkspace);
    else
      g_log.warning("No instrument file name found in the Nexus file");
  }
  // get any spectraMap info
  boost::shared_ptr<IInstrument> localInstrument = localWorkspace->getInstrument();
  SpectraDetectorMap& spectraMap = localWorkspace->mutableSpectraMap();
  nexusFile->readNexusProcessedSpectraMap(spectraMap, localWorkspace, m_spec_min, m_spec_max);
  // Assign the result to the output workspace property
  std::string outputWorkspace = "OutputWorkspace";
  setProperty(outputWorkspace, localWorkspace);
  nexusFile->closeNexusFile();

  return;
}

/// Validates the optional 'spectra to read' properties, if they have been set
void LoadNexusProcessed::checkOptionalProperties()
{
  // from Loadraw2
  Property *specList = getProperty("spectrum_list");
  m_list = !(specList->isDefault());
  Property *specMax = getProperty("spectrum_max");
  m_interval = !(specMax->isDefault());

  // If a multiperiod dataset, ignore the optional parameters (if set) and print a warning
  /*
   if ( m_numberOfPeriods > 1)
   {
   if ( m_list || m_interval )
   {
   m_list = false;
   m_interval = false;
   g_log.warning("Ignoring spectrum properties in this multiperiod dataset");
   }
   }
   */

  // Check validity of spectra list property, if set
  if (m_list)
  {
    m_list = true;
    m_spec_list = getProperty("spectrum_list");
    const int minlist = *min_element(m_spec_list.begin(), m_spec_list.end());
    const int maxlist = *max_element(m_spec_list.begin(), m_spec_list.end());
    if (maxlist > m_numberofspectra || minlist == 0)
    {
      g_log.error("Invalid list of spectra");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }

  // Check validity of spectra range, if set
  if (m_interval)
  {
    m_interval = true;
    m_spec_min = getProperty("spectrum_min");
    m_spec_max = getProperty("spectrum_max");
    if (m_spec_max < m_spec_min || m_spec_max > m_numberofspectra)
    {
      g_log.error("Invalid Spectrum min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }
}

/// Run the sub-algorithm LoadInstrument (as for LoadRaw)
void LoadNexusProcessed::runLoadInstrument(DataObjects::Workspace2D_sptr localWorkspace)
{
  /// Determine the search directory for XML instrument definition files (IDFs)
  /// @param localWorkspace - pointer to the 2D workspace to put the instrument data into
  std::string directoryName = Kernel::ConfigService::Instance().getString(
      "instrumentDefinition.directory");
  if (directoryName.empty())
  {
    // This is the assumed deployment directory for IDFs, where we need to be relative to the
    // directory of the executable, not the current working directory.
    directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve(
        "../Instrument").toString();
  }

  // For Nexus Mantid processed, Instrument XML file name is read from nexus 
  std::string instrumentID = m_instrumentxml;
  std::string fullPathIDF = directoryName + "/" + instrumentID;

  IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("Filename", fullPathIDF);
  loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  try
  {
    loadInst->execute();
  } catch (std::runtime_error& err)
  {
    g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
  }

  // If loading instrument definition file fails, run LoadInstrumentFromNexus instead
  // This does not work at present as the example files do not hold the necessary data
  // but is a place holder. Hopefully the new version of Nexus Muon files should be more
  // complete.
  //if ( ! loadInst->isExecuted() )
  //{
  //runLoadInstrumentFromNexus(localWorkspace);
  //}
}

} // namespace NeXus
} // namespace Mantid
