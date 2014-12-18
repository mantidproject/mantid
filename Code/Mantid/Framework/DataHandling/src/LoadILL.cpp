//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadILL.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/LoadHelper.h"

#include <boost/algorithm/string/predicate.hpp> // boost::starts_with

#include <limits>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cmath>

namespace Mantid
{
  namespace DataHandling
  {

    using namespace Kernel;
    using namespace API;
    using namespace NeXus;

    DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILL);

//---------------------------------------------------
// Private member functions
//---------------------------------------------------

    /**
     * Return the confidence with with this algorithm can load the file
     * @param descriptor A descriptor for the file
     * @returns An integer specifying the confidence level. 0 indicates it will not be used
     */
    int LoadILL::confidence(Kernel::NexusDescriptor & descriptor) const
    {

      // fields existent only at the ILL
      if (descriptor.pathExists("/entry0/wavelength")
          && descriptor.pathExists("/entry0/experiment_identifier")
          && descriptor.pathExists("/entry0/mode") && !descriptor.pathExists("/entry0/dataSD") // This one is for LoadILLIndirect
          && !descriptor.pathExists("/entry0/instrument/VirtualChopper") // This one is for LoadILLReflectometry
              )
      {
        return 80;
      }
      else
      {
        return 0;
      }
    }

    LoadILL::LoadILL() :
        API::IFileLoader<Kernel::NexusDescriptor>()
    {
      m_instrumentName = "";
      m_wavelength = 0;
      m_channelWidth = 0;
      m_numberOfChannels = 0;
      m_numberOfHistograms = 0;
      m_numberOfTubes = 0;
      m_numberOfPixelsPerTube = 0;
      m_monitorElasticPeakPosition = 0;
      m_l1 = 0;
      m_l2 = 0;
      m_supportedInstruments.push_back("IN4");
      m_supportedInstruments.push_back("IN5");
      m_supportedInstruments.push_back("IN6");
    }

    /**
     * Initialise the algorithm
     */
    void LoadILL::init()
    {
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
          "File path of the Data file to load");

      declareProperty(new FileProperty("FilenameVanadium", "", FileProperty::OptionalLoad, ".nxs"),
          "File path of the Vanadium file to load (Optional)");

      declareProperty(
          new WorkspaceProperty<API::MatrixWorkspace>("WorkspaceVanadium", "", Direction::Input,
              PropertyMode::Optional), "Vanadium Workspace file to load (Optional)");

      declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
          "The name to use for the output workspace");

    }

    /**
     * Execute the algorithm
     */
    void LoadILL::exec()
    {
      // Retrieve filename
      std::string filenameData = getPropertyValue("Filename");
      std::string filenameVanadium = getPropertyValue("FilenameVanadium");
      MatrixWorkspace_sptr vanaWS = getProperty("WorkspaceVanadium");

      // open the root node
      NeXus::NXRoot dataRoot(filenameData);
      NXEntry dataFirstEntry = dataRoot.openFirstEntry();

      loadInstrumentDetails(dataFirstEntry);
      loadTimeDetails(dataFirstEntry);

      std::vector<std::vector<int> > monitors = getMonitorInfo(dataFirstEntry);

      initWorkSpace(dataFirstEntry, monitors);

      addAllNexusFieldsAsProperties(filenameData);

      runLoadInstrument(); // just to get IDF contents
      initInstrumentSpecific();

      int calculatedDetectorElasticPeakPosition = getEPPFromVanadium(filenameVanadium, vanaWS);

      loadDataIntoTheWorkSpace(dataFirstEntry, monitors, calculatedDetectorElasticPeakPosition);

      addEnergyToRun();

      // load the instrument from the IDF if it exists
      runLoadInstrument();

      // Set the output workspace property
      setProperty("OutputWorkspace", m_localWorkspace);
    }

    /**
     * Loads Monitor data into an vector of monitor data
     * @return : list of monitor data
     */
    std::vector<std::vector<int> > LoadILL::getMonitorInfo(NeXus::NXEntry& firstEntry)
    {

      std::vector<std::vector<int> > monitorList;

      for (std::vector<NXClassInfo>::const_iterator it = firstEntry.groups().begin();
          it != firstEntry.groups().end(); ++it)
      {

        if (it->nxclass == "NXmonitor" || boost::starts_with(it->nxname, "monitor"))
        {

          g_log.debug() << "Load monitor data from " + it->nxname;

          NXData dataGroup = firstEntry.openNXData(it->nxname + "/data");
          NXInt data = dataGroup.openIntData();
          // load the counts from the file into memory
          data.load();

          std::vector<int> thisMonitor(data(), data() + data.size());
          monitorList.push_back(thisMonitor);
        }
      }
      return monitorList;
    }

    /**
     * Get the elastic peak position (EPP) from a Vanadium Workspace
     * or filename.
     * @return the EPP
     */
    int LoadILL::getEPPFromVanadium(const std::string &filenameVanadium, MatrixWorkspace_sptr vanaWS)
    {
      int calculatedDetectorElasticPeakPosition = -1;

      if (vanaWS != NULL)
      {

        // Check if it has been store on the run object for this workspace
        if (vanaWS->run().hasProperty("EPP"))
        {
          Kernel::Property* prop = vanaWS->run().getProperty("EPP");
          calculatedDetectorElasticPeakPosition = boost::lexical_cast<int>(prop->value());
          g_log.information() << "Using EPP from Vanadium WorkSpace : value =  "
              << calculatedDetectorElasticPeakPosition << "\n";
        }
        else
        {
          g_log.error("No EPP Property in the Vanadium Workspace. Following regular procedure...");
        }
      }
      if (calculatedDetectorElasticPeakPosition == -1 && filenameVanadium != "")
      {
        g_log.information() << "Calculating the elastic peak position from the Vanadium." << std::endl;
        calculatedDetectorElasticPeakPosition = validateVanadium(filenameVanadium);
      }
      return calculatedDetectorElasticPeakPosition;
    }

    /**
     * Set the instrument name along with its path on the nexus file
     */
    void LoadILL::loadInstrumentDetails(NeXus::NXEntry& firstEntry)
    {

      m_instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);

      if (m_instrumentPath == "")
      {
        throw std::runtime_error("Cannot set the instrument name from the Nexus file!");
      }

      m_instrumentName = m_loader.getStringFromNexusPath(firstEntry, m_instrumentPath + "/name");

      if (std::find(m_supportedInstruments.begin(), m_supportedInstruments.end(), m_instrumentName)
          == m_supportedInstruments.end())
      {
        std::string message = "The instrument " + m_instrumentName + " is not valid for this loader!";
        throw std::runtime_error(message);
      }

      g_log.debug() << "Instrument name set to: " + m_instrumentName << std::endl;

    }

    /**
     * Creates the workspace and initialises member variables with
     * the corresponding values
     *
     * @param entry :: The Nexus entry
     * @param monitors :: list of monitors content
     *
     */
    void LoadILL::initWorkSpace(NeXus::NXEntry& entry, const std::vector<std::vector<int> >&monitors)
    {

      // read in the data
      NXData dataGroup = entry.openNXData("data");
      NXInt data = dataGroup.openIntData();

      m_numberOfTubes = static_cast<size_t>(data.dim0());
      m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
      m_numberOfChannels = static_cast<size_t>(data.dim2());
      size_t numberOfMonitors = monitors.size();

      /**
       * IN4 : Rosace detector is now in a different field!
       */
      size_t numberOfTubesInRosace = 0;
      if (m_instrumentName == "IN4")
      {
        NXData dataGroup = entry.openNXData("instrument/Detector_Rosace/data");
        NXInt data = dataGroup.openIntData();
        numberOfTubesInRosace += static_cast<size_t>(data.dim0());
      }

      // dim0 * m_numberOfPixelsPerTube is the total number of detectors
      m_numberOfHistograms = (m_numberOfTubes + numberOfTubesInRosace) * m_numberOfPixelsPerTube;

      g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << std::endl;
      g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << std::endl;
      g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << std::endl;

      // Now create the output workspace
      // total number of spectra + number of monitors,
      // bin boundaries = m_numberOfChannels + 1
      // Z/time dimension
      m_localWorkspace = WorkspaceFactory::Instance().create("Workspace2D",
          m_numberOfHistograms + numberOfMonitors, m_numberOfChannels + 1, m_numberOfChannels);
      m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
      m_localWorkspace->setYUnitLabel("Counts");

    }

    /**
     * Function to do specific instrument stuff
     *
     */
    void LoadILL::initInstrumentSpecific()
    {
      m_l1 = m_loader.getL1(m_localWorkspace);
      // this will be mainly for IN5 (flat PSD detector)
      m_l2 = m_loader.getInstrumentProperty(m_localWorkspace, "l2");
      if (m_l2 == EMPTY_DBL())
      {
        g_log.debug("Calculating L2 from the IDF.");
        m_l2 = m_loader.getL2(m_localWorkspace);
      }
    }

    /**
     * Load the time details from the nexus file.
     * @param entry :: The Nexus entry
     */
    void LoadILL::loadTimeDetails(NeXus::NXEntry& entry)
    {

      m_wavelength = entry.getFloat("wavelength");

      // Monitor can be monitor (IN5) or monitor1 (IN6)
      std::string monitorName;
      if (entry.containsGroup("monitor"))
        monitorName = "monitor";
      else if (entry.containsGroup("monitor1"))
        monitorName = "monitor1";
      else
      {
        std::string message("Cannot find monitor[1] in the Nexus file!");
        g_log.error(message);
        throw std::runtime_error(message);
      }

      m_monitorElasticPeakPosition = entry.getInt(monitorName + "/elasticpeak");

      NXFloat time_of_flight_data = entry.openNXFloat(monitorName + "/time_of_flight");
      time_of_flight_data.load();

      // The entry "monitor/time_of_flight", has 3 fields:
      // channel width , number of channels, Time of flight delay
      m_channelWidth = time_of_flight_data[0];
      //  m_timeOfFlightDelay = time_of_flight_data[2];

      g_log.debug("Nexus Data:");
      g_log.debug() << " ChannelWidth: " << m_channelWidth << std::endl;
      g_log.debug() << " Wavelength: " << m_wavelength << std::endl;
      g_log.debug() << " ElasticPeakPosition: " << m_monitorElasticPeakPosition << std::endl;

    }

    /**
     * Goes through all the fields of the nexus file and add them
     * as parameters in the workspace
     * @param filename :: NeXus file
     */
    void LoadILL::addAllNexusFieldsAsProperties(std::string filename)
    {

      API::Run & runDetails = m_localWorkspace->mutableRun();

      // Open NeXus file
      NXhandle nxfileID;
      NXstatus stat = NXopen(filename.c_str(), NXACC_READ, &nxfileID);

      g_log.debug() << "Starting parsing properties from : " << filename << std::endl;
      if (stat == NX_ERROR)
      {
        g_log.debug() << "convertNexusToProperties: Error loading " << filename;
        throw Kernel::Exception::FileError("Unable to open File:", filename);
      }
      m_loader.addNexusFieldsToWsRun(nxfileID, runDetails);

      g_log.debug() << "End parsing properties from : " << filename << std::endl;

      // Add also "Facility", as asked
      runDetails.addProperty("Facility", std::string("ILL"));

      stat = NXclose(&nxfileID);

    }

    /**
     * Calculates the Energy from the wavelength and adds
     * it at property Ei
     */
    void LoadILL::addEnergyToRun()
    {

      API::Run & runDetails = m_localWorkspace->mutableRun();
      double ei = m_loader.calculateEnergy(m_wavelength);
      runDetails.addProperty<double>("Ei", ei, true); //overwrite

    }

    /**
     * Gets the experimental Elastic Peak Position in the dectector
     * as the value parsed from the nexus file might be wrong.
     *
     * It gets a few spectra in the equatorial line of the detector,
     * sum them up and finds the maximum = the Elastic peak
     *
     * @param data :: spectra data
     * @return detector Elastic Peak Position
     */
    int LoadILL::getDetectorElasticPeakPosition(const NeXus::NXInt &data)
    {

      // j = index in the equatorial line (256/2=128)
      // both index 127 and 128 are in the equatorial line
      size_t j = m_numberOfPixelsPerTube / 2;

      // ignore the first tubes and the last ones to avoid the beamstop
      //get limits in the m_numberOfTubes
      size_t tubesToRemove = m_numberOfTubes / 7;

      std::vector<int> cumulatedSumOfSpectras(m_numberOfChannels, 0);
      for (size_t i = tubesToRemove; i < m_numberOfTubes - tubesToRemove; i++)
      {
        int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
        std::vector<int> thisSpectrum(data_p, data_p + m_numberOfChannels);
        // sum spectras
        std::transform(thisSpectrum.begin(), thisSpectrum.end(), cumulatedSumOfSpectras.begin(),
            cumulatedSumOfSpectras.begin(), std::plus<int>());
      }
      auto it = std::max_element(cumulatedSumOfSpectras.begin(), cumulatedSumOfSpectras.end());

      int calculatedDetectorElasticPeakPosition;
      if (it == cumulatedSumOfSpectras.end())
      {
        g_log.warning() << "No Elastic peak position found! Assuming the EPP in the Nexus file: "
            << m_monitorElasticPeakPosition << std::endl;
        calculatedDetectorElasticPeakPosition = m_monitorElasticPeakPosition;

      }
      else
      {
        calculatedDetectorElasticPeakPosition = static_cast<int>(std::distance(
            cumulatedSumOfSpectras.begin(), it));

        if (calculatedDetectorElasticPeakPosition == 0)
        {
          g_log.warning() << "Elastic peak position is ZERO Assuming the EPP in the Nexus file: "
              << m_monitorElasticPeakPosition << std::endl;
          calculatedDetectorElasticPeakPosition = m_monitorElasticPeakPosition;

        }
        else
        {
          g_log.debug() << "Calculated Detector EPP: " << calculatedDetectorElasticPeakPosition;
          g_log.debug() << " :: Read EPP from the nexus file: " << m_monitorElasticPeakPosition
              << std::endl;
        }
      }
      return calculatedDetectorElasticPeakPosition;

    }

    /**
     * It loads the vanadium nexus file and cross checks it against the
     * data file already loaded (same wavelength and same instrument configuration).
     * If matches looks for the elastic peak in the vanadium file and returns
     * it position.
     *
     * @param filenameVanadium :: The path for the vanadium nexus file.
     * @return The elastic peak position inside the tof channels.
     */
    int LoadILL::validateVanadium(const std::string &filenameVanadium)
    {
      NeXus::NXRoot vanaRoot(filenameVanadium);
      NXEntry vanaFirstEntry = vanaRoot.openFirstEntry();

      double wavelength = vanaFirstEntry.getFloat("wavelength");

      // read in the data
      NXData dataGroup = vanaFirstEntry.openNXData("data");
      NXInt data = dataGroup.openIntData();

      size_t numberOfTubes = static_cast<size_t>(data.dim0());
      size_t numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
      size_t numberOfChannels = static_cast<size_t>(data.dim2());

      if (wavelength != m_wavelength || numberOfTubes != m_numberOfTubes
          || numberOfPixelsPerTube != m_numberOfPixelsPerTube || numberOfChannels != m_numberOfChannels)
      {
        throw std::runtime_error("Vanadium and Data were not collected in the same conditions!");
      }

      data.load();
      int calculatedDetectorElasticPeakPosition = getDetectorElasticPeakPosition(data);
      return calculatedDetectorElasticPeakPosition;
    }

    /**
     * Loads all the spectra into the workspace, including that from the monitor
     *
     * @param entry :: The Nexus entry
     * @param monitors :: List of monitors content
     * @param vanaCalculatedDetectorElasticPeakPosition :: If -1 uses this value as the elastic peak position at the detector.
     *
     */
    void LoadILL::loadDataIntoTheWorkSpace(NeXus::NXEntry& entry,
        const std::vector<std::vector<int> >&monitors, int vanaCalculatedDetectorElasticPeakPosition)
    {

      g_log.debug() << "Loading data into the workspace..." << std::endl;
      // read in the data
      NXData dataGroup = entry.openNXData("data");
      NXInt data = dataGroup.openIntData();
      // load the counts from the file into memory
      data.load();

      // Detector: Find real elastic peak in the detector.
      // Looks for a few elastic peaks on the equatorial line of the detector.
      int calculatedDetectorElasticPeakPosition;
      if (vanaCalculatedDetectorElasticPeakPosition == -1)
        calculatedDetectorElasticPeakPosition = getDetectorElasticPeakPosition(data);
      else
        calculatedDetectorElasticPeakPosition = vanaCalculatedDetectorElasticPeakPosition;

      //set it as a Property
      API::Run & runDetails = m_localWorkspace->mutableRun();
      runDetails.addProperty("EPP", calculatedDetectorElasticPeakPosition);

      double theoreticalElasticTOF = (m_loader.calculateTOF(m_l1, m_wavelength)
          + m_loader.calculateTOF(m_l2, m_wavelength)) * 1e6; //microsecs

      // Calculate the real tof (t1+t2) put it in tof array
      std::vector<double> detectorTofBins(m_numberOfChannels + 1);
      for (size_t i = 0; i < m_numberOfChannels + 1; ++i)
      {
        detectorTofBins[i] = theoreticalElasticTOF
            + m_channelWidth
                * static_cast<double>(static_cast<int>(i) - calculatedDetectorElasticPeakPosition)
            - m_channelWidth / 2; // to make sure the bin is in the middle of the elastic peak
      }

      g_log.information() << "T1+T2 : Theoretical = " << theoreticalElasticTOF;
      g_log.information() << " ::  Calculated bin = ["
          << detectorTofBins[calculatedDetectorElasticPeakPosition] << ","
          << detectorTofBins[calculatedDetectorElasticPeakPosition + 1] << "]" << std::endl;

      // The binning for monitors is considered the same as for detectors
      size_t spec = 0;

      for (auto it = monitors.begin(); it != monitors.end(); ++it)
      {

        m_localWorkspace->dataX(spec).assign(detectorTofBins.begin(), detectorTofBins.end());
        // Assign Y
        m_localWorkspace->dataY(spec).assign(it->begin(), it->end());
        // Assign Error
        MantidVec& E = m_localWorkspace->dataE(spec);
        std::transform(it->begin(), it->end(), E.begin(), LoadILL::calculateError);
        ++spec;

      }

      // Assign calculated bins to first X axis
      size_t firstSpec = spec;
      m_localWorkspace->dataX(firstSpec).assign(detectorTofBins.begin(), detectorTofBins.end());

      Progress progress(this, 0, 1, m_numberOfTubes * m_numberOfPixelsPerTube);
      //size_t spec = 0;
      for (size_t i = 0; i < m_numberOfTubes; ++i)
      {
        for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j)
        {
          if (spec > firstSpec)
          {
            // just copy the time binning axis to every spectra
            m_localWorkspace->dataX(spec) = m_localWorkspace->readX(firstSpec);
          }
          // Assign Y
          int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
          m_localWorkspace->dataY(spec).assign(data_p, data_p + m_numberOfChannels);

          // Assign Error
          MantidVec& E = m_localWorkspace->dataE(spec);
          std::transform(data_p, data_p + m_numberOfChannels, E.begin(), LoadILL::calculateError);

          ++spec;
          progress.report();
        }
      }

      g_log.debug() << "Loading data into the workspace: DONE!" << std::endl;

      /**
       * IN4 Low angle and high angle have been split!
       */
      if (m_instrumentName == "IN4")
      {
        g_log.debug() << "Loading data into the workspace: IN4 Rosace!" << std::endl;
        // read in the data
        NXData dataGroup = entry.openNXData("instrument/Detector_Rosace/data");
        NXInt data = dataGroup.openIntData();
        auto numberOfTubes = static_cast<size_t>(data.dim0());
        // load the counts from the file into memory
        data.load();

        Progress progress(this, 0, 1, numberOfTubes * m_numberOfPixelsPerTube);
        for (size_t i = 0; i < numberOfTubes; ++i)
        {
          for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j)
          {
            // just copy the time binning axis to every spectra
            m_localWorkspace->dataX(spec) = m_localWorkspace->readX(firstSpec);

            // Assign Y
            int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
            m_localWorkspace->dataY(spec).assign(data_p, data_p + m_numberOfChannels);

            // Assign Error
            MantidVec& E = m_localWorkspace->dataE(spec);
            std::transform(data_p, data_p + m_numberOfChannels, E.begin(), LoadILL::calculateError);

            ++spec;
            progress.report();
          }
        }

      }

    }

    /**
     * Run the Child Algorithm LoadInstrument.
     */
    void LoadILL::runLoadInstrument()
    {

      IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

      // Now execute the Child Algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->setPropertyValue("InstrumentName", m_instrumentName);
        loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
        loadInst->execute();
      } catch (...)
      {
        g_log.information("Cannot load the instrument definition.");
      }
    }

  } // namespace DataHandling
} // namespace Mantid
