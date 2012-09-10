/*WIKI* 

Some instrument definition file ([[InstrumentDefinitionFile|IDF]]) positions are only approximately correct and the true positions are located within data files. This algorithm reads the detector positioning from the supplied datafile and updates the instrument accordingly. It currently supports ISIS Raw and NeXus files.

It is assumed that the positions specified in the raw file are all with respect to the a coordinate system defined with its origin at the sample position.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/UpdateInstrumentFromFile.h"
#include "MantidDataHandling/LoadRawHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "LoadRaw/isisraw2.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidNexusCPP/NeXusException.hpp"
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
  namespace DataHandling
  {

    DECLARE_ALGORITHM(UpdateInstrumentFromFile)

    /// Sets documentation strings for this algorithm
    void UpdateInstrumentFromFile::initDocs()
    {
      this->setWikiSummary("Update detector positions initially loaded in from Instrument Definition File ([[InstrumentDefinitionFile|IDF]]) from information the given file. Note doing this will results in a slower performance (likely slightly slower performance) compared to specifying the correct detector positions in the IDF in the first place. It is assumed that the positions specified in the raw file are all with respect to the a coordinate system defined with its origin at the sample position.  Note that this algorithm moves the detectors without subsequent rotation, hence this means that detectors may not for example face the sample perfectly after this algorithm has been applied. ");
      this->setOptionalMessage("Updates detector positions initially loaded in from the Instrument Definition File (IDF) with information from the provided file. Currently supports RAW and ISIS NeXus.");
    }
    
    using namespace Kernel;
    using namespace API;
    using Geometry::Instrument;
    using Geometry::Instrument_sptr;
    using Geometry::IDetector_sptr;
    using Kernel::V3D;

    /// Empty default constructor
    UpdateInstrumentFromFile::UpdateInstrumentFromFile()
    {}

    /// Initialisation method.
    void UpdateInstrumentFromFile::init()
    {
      // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
      declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
        "The name of the workspace in which to store the imported instrument");

      std::vector<std::string> exts;
      exts.push_back(".raw");
      exts.push_back(".s*");
      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The filename of the input file.\n"
        "Currently supports RAW and ISIS NeXus."
        );
      declareProperty("MoveMonitors", false, 
                      "If true the positions of any detectors marked as monitors "
                      "in the IDF will be moved also");
      declareProperty("IgnorePhi", false, 
                      "If true the phi values form the file will be ignored ");
    }

    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    *
    *  @throw FileError Thrown if unable to parse XML file
    */
    void UpdateInstrumentFromFile::exec()
    {
      // Retrieve the filename from the properties
      const std::string filename = getPropertyValue("Filename");
      m_workspace = getProperty("Workspace");
      if(!m_workspace->getInstrument())
      {
        throw std::runtime_error("Input workspace has no defined instrument");
      }

      // Check the file type
      boost::scoped_ptr<LoadRawHelper> rawCheck(new LoadRawHelper()); 
      if( rawCheck->fileCheck(filename) > 0 )
      {
        updateFromRaw(filename);
      }
      // Assume it is a NeXus file for now 
      // @todo: When Nexus is merged use the file checker also
      else
      {
        updateFromNeXus(filename);
      }
    }

    /**
    * Update the detector information from a raw file
    * @param filename :: The input filename
    */
    void UpdateInstrumentFromFile::updateFromRaw(const std::string & filename)
    {
      ISISRAW2 iraw;
      if (iraw.readFromFile(filename.c_str(),false) != 0)
      {
        g_log.error("Unable to open file " + filename);
        throw Exception::FileError("Unable to open File:" , filename);
      }

      const int32_t numDetector = iraw.i_det;
      std::vector<int32_t> detID(iraw.udet, iraw.udet + numDetector);
      std::vector<float> l2(iraw.len2, iraw.len2 + numDetector);
      std::vector<float> theta(iraw.tthe, iraw.tthe + numDetector);
      // Is ut01 (=phi) present? Sometimes an array is present but has wrong values e.g.all 1.0 or all 2.0
      bool phiPresent = iraw.i_use>0 && iraw.ut[0]!= 1.0 && iraw.ut[0] !=2.0; 
      std::vector<float> phi(0);
      if( phiPresent )
      {
        phi = std::vector<float>(iraw.ut, iraw.ut + numDetector);
      }
      else
      {
        phi = std::vector<float>(numDetector, 0.0);
      }
      g_log.information() << "Setting detector postions from RAW file.\n";
      setDetectorPositions(detID, l2, theta, phi);
    }

    /**
    * Update the detector information from a NeXus file
    * @param filename :: The input filename
    */
    void UpdateInstrumentFromFile::updateFromNeXus(const std::string & filename)
    {
      try
      {
        ::NeXus::File file(filename);
      }
      catch(::NeXus::Exception&)
      {
        throw std::runtime_error("Input file does not look like an ISIS NeXus file.");
      }
      ::NeXus::File nxFile(filename);
      try
      {
        nxFile.openPath("raw_data_1/isis_vms_compat");
      }
      catch(::NeXus::Exception&)
      {
        try
        {
          nxFile.openPath("entry/isis_vms_compat"); // Could be original event file.
        }
        catch(::NeXus::Exception&)
        {
          throw std::runtime_error("Unknown NeXus flavour. Cannot update instrument positions.");
        }
      }
      // Det ID
      std::vector<int32_t> detID;
      nxFile.openData("UDET");
      nxFile.getData(detID);
      nxFile.closeData();
      // Position information
      std::vector<float> l2, theta,phi;
      nxFile.openData("LEN2");
      nxFile.getData(l2);
      nxFile.closeData();
      nxFile.openData("TTHE");
      nxFile.getData(theta);
      nxFile.closeData();
      nxFile.openData("UT01");
      nxFile.getData(phi);
      nxFile.closeData();

      g_log.information() << "Setting detector postions from NeXus file.\n";
      setDetectorPositions(detID, l2, theta, phi);
    }

    /**
     * Set the detector positions given the r,theta and phi.
     * @param detID :: A vector of detector IDs
     * @param l2 :: A vector of l2 distances
     * @param theta :: A vector of theta distances
     * @param phi :: A vector of phi values
     */
    void UpdateInstrumentFromFile::setDetectorPositions(const std::vector<int32_t> & detID, const std::vector<float> & l2,
                                                        const std::vector<float> & theta, const std::vector<float> & phi)
      {
        const bool ignorePhi = getProperty("IgnorePhi");
        const bool moveMonitors = getProperty("MoveMonitors");
        const bool ignoreMonitors(!moveMonitors);
        const int numDetector = static_cast<int>(detID.size());
        Geometry::ParameterMap & pmap = m_workspace->instrumentParameters();
        Geometry::Instrument_const_sptr instrument = m_workspace->getInstrument();
        g_log.information() << "Setting new positions for " << numDetector << " detectors\n";
        for (int i = 0; i < numDetector; ++i)
        {
          try
          {
            Geometry::IDetector_const_sptr det = instrument->getDetector(detID[i]);
            if( ignoreMonitors && det->isMonitor() ) continue;
            V3D parentPos;
            if( det->getParent() ) parentPos = det->getParent()->getPos();
            Kernel::V3D pos;
            if (!ignorePhi)
            {
              pos.spherical(l2[i], theta[i], phi[i]);
            }
            else
            {
              double r,t,p;
              det->getPos().getSpherical(r,t,p);
              pos.spherical(l2[i], theta[i], p);
            }
            // Set new relative position
            Kernel::V3D r = pos-parentPos;
            Kernel::Quat q = det->getParent()->getRotation();
            q.inverse();
            q.rotate(r);
            pmap.addV3D(det.get(), "pos", r);
          }
          catch (Kernel::Exception::NotFoundError&)
          {
          }
          progress(static_cast<double>(i)/numDetector);
        }  

      }

  } // namespace DataHandling
} // namespace Mantid
