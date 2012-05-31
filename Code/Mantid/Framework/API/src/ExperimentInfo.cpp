#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <map>
#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <Poco/RegularExpression.h>
#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/SAXParser.h>
#include <fstream>
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidAPI/InstrumentDataService.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Poco::XML;

namespace Mantid
{
namespace API
{

  Kernel::Logger& ExperimentInfo::g_log = Kernel::Logger::get("ExperimentInfo");

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ExperimentInfo::ExperimentInfo()
  : m_sample(),
    m_run(),
    m_parmap(new ParameterMap()),
    sptr_instrument(new Instrument())
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ExperimentInfo::~ExperimentInfo()
  {
  }


  //---------------------------------------------------------------------------------------
  /** Copy the experiment info data from another ExperimentInfo instance,
   * e.g. a MatrixWorkspace.
   * @param other :: the source from which to copy ExperimentInfo
   */
  void ExperimentInfo::copyExperimentInfoFrom(const ExperimentInfo * other)
  {
    m_sample = other->m_sample;
    m_run = other->m_run;
    this->setInstrument(other->getInstrument());
  }

  //---------------------------------------------------------------------------------------
  /** Clone this ExperimentInfo class into a new one
   */
  ExperimentInfo * ExperimentInfo::cloneExperimentInfo()
  {
    ExperimentInfo * out = new ExperimentInfo();
    out->copyExperimentInfoFrom(this);
    return out;
  }


  //---------------------------------------------------------------------------------------
  /** Set the instrument
  *
  * @param instr :: Shared pointer to an instrument.
  */
  void ExperimentInfo::setInstrument(const Instrument_const_sptr& instr)
  {
    if (instr->isParametrized())
    {
      sptr_instrument = instr->baseInstrument();
      m_parmap = instr->getParameterMap();
    }
    else
    {
      sptr_instrument = instr;
    }
  }


  //---------------------------------------------------------------------------------------
  /** Get a shared pointer to the parametrized instrument associated with this workspace
  *
  *  @return The instrument class
  */
  Instrument_const_sptr ExperimentInfo::getInstrument()const
  {
    return Geometry::ParComponentFactory::createInstrument(sptr_instrument, m_parmap);
  }

  //---------------------------------------------------------------------------------------
  /**  Returns a new copy of the instrument parameters
  *    @return a (new) copy of the instruments parameter map
  */
  Geometry::ParameterMap& ExperimentInfo::instrumentParameters()
  {
    //TODO: Here duplicates cow_ptr. Figure out if there's a better way

    // Use a double-check for sharing so that we only
    // enter the critical region if absolutely necessary
    if (!m_parmap.unique())
    {
      PARALLEL_CRITICAL(cow_ptr_access)
      {
        // Check again because another thread may have taken copy
        // and dropped reference count since previous check
        if (!m_parmap.unique())
        {
          ParameterMap_sptr oldData=m_parmap;
          m_parmap.reset();
          m_parmap = ParameterMap_sptr(new ParameterMap(*oldData));
        }
      }
    }

    return *m_parmap;
    //return m_parmap.access(); //old cow_ptr thing
  }


  //---------------------------------------------------------------------------------------
  /**  Returns a const reference to the instrument parameters.
  *    @return a const reference to the instrument ParameterMap.
  */
  const Geometry::ParameterMap& ExperimentInfo::instrumentParameters() const
  {
    return *m_parmap.get();
  }

  //---------------------------------------------------------------------------------------
  /**  Returns a const reference to the instrument parameters.
  *    @return a const reference to the instrument ParameterMap.
  */
  const Geometry::ParameterMap& ExperimentInfo::constInstrumentParameters() const
  {
    return *m_parmap;
  }

  /// Used for storing info about "r-position", "t-position" and "p-position" parameters
  /// as all parameters are processed
  struct m_PositionEntry
  { m_PositionEntry(std::string& name, double val) : paramName(name), value(val) {}
    std::string paramName;
    double value; };


  //---------------------------------------------------------------------------------------
  /** Add parameters to the instrument parameter map that are defined in instrument
  *   definition file and for which logfile data are available. Logs must be loaded
  *   before running this method.
  */
  void ExperimentInfo::populateInstrumentParameters()
  {
    // Get instrument and sample
    boost::shared_ptr<const Instrument> instrument = getInstrument()->baseInstrument();
    Instrument* inst = const_cast<Instrument*>(instrument.get());

    // Get the data in the logfiles associated with the raw data
    const std::vector<Kernel::Property*>& logfileProp = run().getLogData();


    // Get pointer to parameter map that we may add parameters to and information about
    // the parameters that my be specified in the instrument definition file (IDF)
    Geometry::ParameterMap& paramMap = instrumentParameters();
    const std::multimap<std::string, boost::shared_ptr<XMLlogfile> >& paramInfoFromIDF = inst->getLogfileCache();


    // iterator to browse through the multimap: paramInfoFromIDF
    std::multimap<std::string, boost::shared_ptr<XMLlogfile> > :: const_iterator it;
    std::pair<std::multimap<std::string, boost::shared_ptr<XMLlogfile> >::const_iterator,
      std::multimap<std::string, boost::shared_ptr<XMLlogfile> >::const_iterator> ret;

    // In order to allow positions to be set with r-position, t-position and p-position parameters
    // The idea is here to simply first check if parameters with names "r-position", "t-position"
    // and "p-position" are encounted then at the end of this method act on this
    std::set<const IComponent*> rtp_positionComp;
    std::multimap<const IComponent*, m_PositionEntry > rtp_positionEntry;

    // loop over all logfiles and see if any of these are associated with parameters in the
    // IDF

    size_t N = logfileProp.size();
    for (size_t i = 0; i < N; i++)
    {
      // Get the name of the timeseries property

      std::string logName = logfileProp[i]->name();

      // See if filenamePart matches any logfile-IDs in IDF. If this add parameter to parameter map

      ret = paramInfoFromIDF.equal_range(logName);
      for (it=ret.first; it!=ret.second; ++it)
      {
        double value = ((*it).second)->createParamValue(static_cast<Kernel::TimeSeriesProperty<double>*>(logfileProp[i]));

        // special cases of parameter names

        std::string paramN = ((*it).second)->m_paramName;
        if ( paramN.compare("x")==0 || paramN.compare("y")==0 || paramN.compare("z")==0 )
          paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
        else if ( paramN.compare("rot")==0 || paramN.compare("rotx")==0 || paramN.compare("roty")==0 || paramN.compare("rotz")==0 )
        {
          paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
        }
        else if ( paramN.compare("r-position")==0 || paramN.compare("t-position")==0 || paramN.compare("p-position")==0 )
        {
          rtp_positionComp.insert(((*it).second)->m_component);
          rtp_positionEntry.insert(
            std::pair<const IComponent*, m_PositionEntry >(
              ((*it).second)->m_component, m_PositionEntry(paramN, value)));
        }
        else
          paramMap.addDouble(((*it).second)->m_component, paramN, value);
      }
    }

    // Check if parameters have been specified using the 'value' attribute rather than the 'logfile-id' attribute
    // All such parameters have been stored using the key = "".
    ret = paramInfoFromIDF.equal_range("");
    Kernel::TimeSeriesProperty<double>* dummy = NULL;
    for (it = ret.first; it != ret.second; ++it)
    {
      std::string paramN = ((*it).second)->m_paramName;
      std::string category = ((*it).second)->m_type;

      // if category is sting no point in trying to generate a double from parameter
      double value = 0.0;
      if ( category.compare("string") != 0 )
        value = ((*it).second)->createParamValue(dummy);

      if ( category.compare("fitting") == 0 )
      {
        std::ostringstream str;
        str << value << " , " << ((*it).second)->m_fittingFunction << " , " << paramN << " , " << ((*it).second)->m_constraint[0] << " , "
          << ((*it).second)->m_constraint[1] << " , " << ((*it).second)->m_penaltyFactor << " , "
          << ((*it).second)->m_tie << " , " << ((*it).second)->m_formula << " , "
          << ((*it).second)->m_formulaUnit << " , " << ((*it).second)->m_resultUnit << " , " << (*(((*it).second)->m_interpolation));
        paramMap.add("fitting",((*it).second)->m_component, paramN, str.str());
      }
      else if ( category.compare("string") == 0 )
      {
        paramMap.addString(((*it).second)->m_component, paramN, ((*it).second)->m_value);
      }
      else
      {
        if (paramN.compare("x") == 0 || paramN.compare("y") == 0 || paramN.compare("z") == 0)
          paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
        else if ( paramN.compare("rot")==0 || paramN.compare("rotx")==0 || paramN.compare("roty")==0 || paramN.compare("rotz")==0 )
          paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
        else if ( paramN.compare("r-position")==0 || paramN.compare("t-position")==0 || paramN.compare("p-position")==0 )
        {
          rtp_positionComp.insert(((*it).second)->m_component);
          rtp_positionEntry.insert(
            std::pair<const IComponent*, m_PositionEntry >(
              ((*it).second)->m_component, m_PositionEntry(paramN, value)));
        }
        else
          paramMap.addDouble(((*it).second)->m_component, paramN, value);
      }
    }

    // check if parameters with names "r-position", "t-position"
    // and "p-position" were encounted
    std::pair<std::multimap<const IComponent*, m_PositionEntry >::iterator,
      std::multimap<const IComponent*, m_PositionEntry >::iterator> retComp;
    double deg2rad = (M_PI/180.0);
    std::set<const IComponent*>::iterator itComp;
    std::multimap<const IComponent*, m_PositionEntry > :: const_iterator itRTP;
    for (itComp=rtp_positionComp.begin(); itComp!=rtp_positionComp.end(); ++itComp)
    {
      retComp = rtp_positionEntry.equal_range(*itComp);
      bool rSet = false;
      double rVal=0.0;
      double tVal=0.0;
      double pVal=0.0;
      for (itRTP = retComp.first; itRTP!=retComp.second; ++itRTP)
      {
        std::string paramN = ((*itRTP).second).paramName;
        if ( paramN.compare("r-position")==0 )
        {
          rSet = true;
          rVal = ((*itRTP).second).value;
        }
        if ( paramN.compare("t-position")==0 )
        {
          tVal = deg2rad*((*itRTP).second).value;
        }
        if ( paramN.compare("p-position")==0 )
        {
          pVal = deg2rad*((*itRTP).second).value;
        }
      }
      if ( rSet )
      {
        // convert spherical coordinates to cartesian coordinate values
        double x = rVal*sin(tVal)*cos(pVal);
        double y = rVal*sin(tVal)*sin(pVal);
        double z = rVal*cos(tVal);

        paramMap.addPositionCoordinate(*itComp, "x", x);
        paramMap.addPositionCoordinate(*itComp, "y", y);
        paramMap.addPositionCoordinate(*itComp, "z", z);
      }
    }
  }

  //---------------------------------------------------------------------------------------
  /** Get a constant reference to the Sample associated with this workspace.
  * @return const reference to Sample object
  */
  const  Sample& ExperimentInfo::sample() const
  {
    return *m_sample;
  }

  /** Get a reference to the Sample associated with this workspace.
  *  This non-const method will copy the sample if it is shared between
  *  more than one workspace, and the reference returned will be to the copy.
  *  Can ONLY be taken by reference!
  * @return reference to sample object
  */
  Sample& ExperimentInfo::mutableSample()
  {
    return m_sample.access();
  }


  //---------------------------------------------------------------------------------------
  /** Get a constant reference to the Run object associated with this workspace.
  * @return const reference to run object
  */
  const Run& ExperimentInfo::run() const
  {
    return *m_run;
  }

  /** Get a reference to the Run object associated with this workspace.
  *  This non-const method will copy the Run object if it is shared between
  *  more than one workspace, and the reference returned will be to the copy.
  *  Can ONLY be taken by reference!
  * @return reference to Run object
  */
  Run& ExperimentInfo::mutableRun()
  {
    return m_run.access();
  }
  //---------------------------------------------------------------------------------------
  /** Utility method to get the run number
   *
   * @return the run number (int) or 0 if not found.
   */
  int ExperimentInfo::getRunNumber() const
  {
    if (!m_run->hasProperty("run_number"))
    {
      // No run_number property, default to 0
      return 0;
    }
    else
    {
      Property * prop = m_run->getProperty("run_number");
      if (prop)
      {
        // Use the string representation. That way both a string and a number property will work.
        int val;
        if (Strings::convert(prop->value(), val))
          return val;
        else
          return 0;
      }
    }
    return 0;
  }

  // used to terminate SAX process
  class DummyException {
  public:
    std::string m_validFrom;
    std::string m_validTo;
    DummyException(std::string validFrom, std::string validTo)
      : m_validFrom(validFrom), m_validTo(validTo) {}
  };

  // SAX content handler for grapping stuff quickly from IDF
  class myContentHandler : public Poco::XML::ContentHandler
  {
    virtual void startElement(const XMLString &, const XMLString & localName, const XMLString &, const Attributes & attrList )
    {
      if (localName == "instrument" )
      {
        throw DummyException(static_cast<std::string>(attrList.getValue("","valid-from")),
            static_cast<std::string>(attrList.getValue("","valid-to")));
      }
    }
    virtual void endElement(const XMLString &, const XMLString &, const XMLString & ) {}
    virtual void startDocument() {}
    virtual void endDocument() {}
    virtual void characters(const XMLChar [], int , int ) {}
    virtual void endPrefixMapping(const XMLString & ) {}
    virtual void ignorableWhitespace(const XMLChar [], int , int ) {}
    virtual void processingInstruction(const XMLString & , const XMLString & ) {}
    virtual void setDocumentLocator(const Locator * ) {}
    virtual void skippedEntity(const XMLString & ) {}
    virtual void startPrefixMapping(const XMLString & , const XMLString & ) {}
  };

  //---------------------------------------------------------------------------------------
  /** Return from an IDF the values of the valid-from and valid-to attributes
  *
  *  @param IDFfilename :: Full path of an IDF
  *  @param[out] outValidFrom :: Used to return valid-from date
  *  @param[out] outValidTo :: Used to return valid-to date
  */
  void ExperimentInfo::getValidFromTo(const std::string& IDFfilename, std::string& outValidFrom,
    std::string& outValidTo)
  {
        SAXParser pParser;
        // Create on stack to ensure deletion. Relies on pParser also being local variable.
        myContentHandler  conHand;
        pParser.setContentHandler(&conHand);

        try
        {
          pParser.parse(IDFfilename);
        }
        catch(DummyException &e)
        {
          outValidFrom = e.m_validFrom;
          outValidTo = e.m_validTo;
        }
        catch(...)
        {
          // should throw some sensible here
        }
  }

  //---------------------------------------------------------------------------------------
  /** Return workspace start date as an ISO 8601 string. If this info not stored in workspace the
  *   method returns current date.
  *
  *  @return workspace start date as a string
  */
  std::string ExperimentInfo::getWorkspaceStartDate()
  {
    std::string date;
    if ( m_run->hasProperty("run_start") )
      date = m_run->getProperty("run_start")->value();
    else
    {
      g_log.information("run_start not stored in workspace. Default to current date.");
      date = Kernel::DateAndTime::getCurrentTime().toISO8601String();
    }
    return date;
  }

  //---------------------------------------------------------------------------------------
  /** A given instrument may have multiple IDFs associated with it. This method return an
  *  identifier which identify a given IDF for a given instrument. An IDF filename is
  *  required to be of the form IDFname + _Definition + Identifier + .xml, the identifier
  *  then is the part of a filename that identifies the IDF valid at a given date.
  *
  *  If several IDF files are valid at the given date the file with the most recent from
  *  date is selected. If no such files are found the file with the latest from date is 
  *  selected.
  *
  *  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
  *  @param date :: ISO 8601 date
  *  @return full path of IDF
  */
  std::string ExperimentInfo::getInstrumentFilename(const std::string& instrumentName, const std::string& date)
  {
    g_log.debug() << "Looking for instrument XML file for " << instrumentName << " that is valid on '" << date << "'\n";
    // Lookup the instrument (long) name
    std::string instrument(Kernel::ConfigService::Instance().getInstrument(instrumentName).name());

    // Get the search directory for XML instrument definition files (IDFs)
    std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();

    Poco::RegularExpression regex(instrument+"_Definition.*\\.xml", Poco::RegularExpression::RE_CASELESS );
    Poco::DirectoryIterator end_iter;
    DateAndTime d(date);
    bool foundGoodFile = false; // True if we have found a matching file (valid at the given date)
    std::string mostRecentIDF; // store most recently starting matching IDF if found, else most recently starting IDF.
    DateAndTime refDate("1900-01-31 23:59:00"); // used to help determine the most recently starting IDF, if none match 
    DateAndTime refDateGoodFile("1900-01-31 23:59:00"); // used to help determine the most recently starting matching IDF 
    for ( Poco::DirectoryIterator dir_itr(directoryName); dir_itr != end_iter; ++dir_itr )
    {
      if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

      std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();
      if ( regex.match(l_filenamePart) )
      {
        g_log.debug() << "Found file: '" << dir_itr->path() << "'\n";
        std::string validFrom, validTo;
        getValidFromTo(dir_itr->path(), validFrom, validTo);
        g_log.debug() << "File '" << dir_itr->path() << " valid dates: from '" << validFrom << "' to '" << validTo << "'\n";
        DateAndTime from(validFrom);
        // Use a default valid-to date if none was found.
        DateAndTime to;
        if (validTo.length() > 0)
          to.setFromISO8601(validTo);
        else
          to.setFromISO8601("2100-01-01");

        if ( from <= d && d <= to )
        {
          if( from > refDateGoodFile ) 
          { // We'd found a matching file more recently starting than any other matching file found
            foundGoodFile = true;
            refDateGoodFile = from;
            mostRecentIDF = dir_itr->path();
          }
        }
        if ( !foundGoodFile && ( from > refDate ) )
        {  // Use most recently starting file, in case we don't find a matching file.
          refDate = from;
          mostRecentIDF = dir_itr->path();
        }
      }
    }

    return mostRecentIDF;
  }

  /** A given instrument may have multiple IDFs associated with it. This method return an
  *  identifier which identify a given IDF for a given instrument. An IDF filename is
  *  required to be of the form IDFname + _Definition + Identifier + .xml, the identifier
  *  then is the part of a filename that identifies the IDF valid at the current date.
  *
  *  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
  *  @return full path of IDF
  */
  std::string ExperimentInfo::getInstrumentFilename(const std::string &instrumentName)
  {
      // Just use the current date
      const std::string date = Kernel::DateAndTime::getCurrentTime().toISO8601String();
      return ExperimentInfo::getInstrumentFilename(instrumentName, date);
  }


  //--------------------------------------------------------------------------------------------
  /** Save the object to an open NeXus file.
   * @param file :: open NeXus file
   */
  void ExperimentInfo::saveExperimentInfoNexus(::NeXus::File * file) const
  {
    Instrument_const_sptr instrument = getInstrument();

    // Start with instrument info (name, file, full XML text)
    file->makeGroup("instrument", "NXinstrument", true);
    file->putAttr("version", 1);
    file->writeData("name", instrument->getName() );

    // XML contents of instrument, as a NX note
    file->makeGroup("instrument_xml", "NXnote", true);
    file->writeData("data", instrument->getXmlText() );
    file->writeData("type", "text/xml"); // mimetype
    file->writeData("description", "XML contents of the instrument IDF file.");
    file->closeGroup();

    file->writeData("instrument_source", Poco::Path(instrument->getFilename()).getFileName());

    // Now the parameter map, as a NXnote
    const Geometry::ParameterMap& params = constInstrumentParameters();
    std::string str = params.asString();
    file->makeGroup("instrument_parameter_map", "NXnote", true);
    file->writeData("data", str);
    file->writeData("type", "text/plain"); // mimetype
    file->closeGroup();

    // Add physical detector and monitor data
    std::vector<detid_t> detectorIDs;
    std::vector<detid_t> detmonIDs;
    detectorIDs = instrument->getDetectorIDs( true );
    detmonIDs = instrument->getDetectorIDs( false );
    if( !detmonIDs.empty() )
    {
      // Add detectors group
      file->makeGroup("physical_detectors","NXdetector", true);
      file->writeData("number_of_detectors", uint64_t(detectorIDs.size()) );
      saveDetectorSetInfoToNexus ( file, detectorIDs );
      file->closeGroup(); // detectors

      // Create Monitor IDs vector
      std::vector<IDetector_const_sptr> detmons;
      detmons = instrument->getDetectors( detmonIDs );
      std::vector<detid_t> monitorIDs;
      for (size_t i=0; i < detmonIDs.size(); i++) 
      {
        if( detmons[i]->isMonitor()) monitorIDs.push_back( detmonIDs[i] );
      }

      // Add Monitors group
      file->makeGroup("physical_monitors","NXmonitor", true);
      file->writeData("number_of_monitors", uint64_t(monitorIDs.size()) );
      saveDetectorSetInfoToNexus ( file, monitorIDs );
      file->closeGroup(); // monitors
    }

    file->closeGroup(); // (close the instrument group)

    m_sample->saveNexus(file, "sample");
    m_run->saveNexus(file, "logs");
  }

  /* A private helper function so save information about a set of detectors to Nexus
  *  @param file :: open Nexus file ready to recieve the info about the set of detectors
  *                 a group must be open that has only one call of this function.
  *  @detIDs the dectector IDs of the detectors belonging to the set
  */
  void ExperimentInfo::saveDetectorSetInfoToNexus (::NeXus::File * file, std::vector<detid_t> detIDs ) const
  { 
    Instrument_const_sptr instrument = getInstrument();

    size_t nDets = detIDs.size();
    if( nDets == 0) return;
    std::vector<IDetector_const_sptr> detectors;
    detectors = instrument->getDetectors( detIDs );

    Geometry::IObjComponent_const_sptr sample = instrument->getSample();
    Kernel::V3D sample_pos;
    if(sample) sample_pos = sample->getPos();

    std::vector<double> a_angles( nDets );
    std::vector<double> p_angles( nDets );
    std::vector<double> distances( nDets );

    for (size_t i=0; i < nDets; i++)
    {
      if( sample) 
      {
        Kernel::V3D pos = detectors[i]->getPos() - sample_pos;
        pos.getSpherical( distances[i], p_angles[i], a_angles[i]);
      } else {
        a_angles[i] = detectors[i]->getPhi()*180.0/M_PI;
      }
    }
    file->writeData("detector_number", detIDs);
    file->writeData("azimuthal_angle", a_angles);
    file->openData("azimuthal_angle");
    file->putAttr("units","degree");
    file->closeData();
    if(sample) 
    {
      file->writeData("polar_angle", p_angles);
      file->openData("polar_angle");
      file->putAttr("units","degree");
      file->closeData();
      file->writeData("distance", distances);
      file->openData("distance");
      file->putAttr("units","metre");
      file->closeData();
    }

  }

  //--------------------------------------------------------------------------------------------
  /** Load the object from an open NeXus file.
   * @param file :: open NeXus file
   * @param[out] parameterStr :: special string for all the parameters.
   *             Feed that to ExperimentInfo::readParameterMap() after the instrument is done.
   */
  void ExperimentInfo::loadExperimentInfoNexus(::NeXus::File * file, std::string & parameterStr)
  {
    std::string instrumentName;
    std::string instrumentXml;
    std::string instrumentFilename;

    // First, the sample and then the logs
    int sampleVersion = m_sample.access().loadNexus(file, "sample");
    if (sampleVersion == 0)
    {
      // Old-style (before Sep-9-2011) NXS processed
      // sample field contains both the logs and the sample details
      file->openGroup("sample", "NXsample");
      this->mutableRun().loadNexus(file, "");
      file->closeGroup();
    }
    else
    {
      // Newer style: separate "logs" field for the Run object
      this->mutableRun().loadNexus(file, "logs");
    }

    // Now the instrument source
    instrumentXml = "";
    instrumentName = "";

    // Try to get the instrument file
    file->openGroup("instrument", "NXinstrument");
    file->readData("name", instrumentName);

    int version = 0;
    try { file->getAttr("version", version); } catch (...) {}
    if (version == 0)
    { // Old style: instrument_source and instrument_parameter_map were at the same level as instrument.
      file->closeGroup();
    }

    file->readData("instrument_source", instrumentFilename);
    file->openGroup("instrument_parameter_map", "NXnote");
    file->readData("data", parameterStr);
    file->closeGroup();

    if (version > 0)
    {
      file->openGroup("instrument_xml", "NXnote");
      file->readData("data", instrumentXml );
      file->closeGroup();
      file->closeGroup();
    }

    instrumentFilename = Strings::strip(instrumentFilename);
    instrumentXml = Strings::strip(instrumentXml);
    instrumentName = Strings::strip(instrumentName);
    if (instrumentXml.empty() && !instrumentName.empty())
    {
      // XML was not included or was empty.
      // Use the instrument name to find the file
      try
      {
        std::string filename = getInstrumentFilename(instrumentName, getWorkspaceStartDate() );
        // And now load the contents
        instrumentFilename = filename;
        instrumentXml = Strings::loadFile(filename);
      }
      catch (std::exception & e)
      {
        g_log.error() << "Error loading instrument IDF file for '" << instrumentName << "'." << std::endl;
        g_log.error() << e.what() << std::endl;
      }
    }
    else
    {
      // The filename in the file = just bare file.
      // So Get the full path back to the instrument directory.
      instrumentFilename = ConfigService::Instance().getInstrumentDirectory() + "/" + instrumentFilename;
      g_log.debug() << "Using instrument IDF XML text contained in .nxs file." << std::endl;
    }


    // ---------- Now parse that XML to make the instrument -------------------
    if (!instrumentXml.empty() && !instrumentName.empty())
    {
      InstrumentDefinitionParser parser;
      parser.initialize(instrumentFilename, instrumentName, instrumentXml);
      std::string instrumentNameMangled = parser.getMangledName();
      Instrument_sptr instr;
      // Check whether the instrument is already in the InstrumentDataService
      if ( InstrumentDataService::Instance().doesExist(instrumentNameMangled) )
      {
        // If it does, just use the one from the one stored there
        instr = InstrumentDataService::Instance().retrieve(instrumentNameMangled);
      }
      else
      {
        // Really create the instrument
        instr = parser.parseXML(NULL);
        // Add to data service for later retrieval
        InstrumentDataService::Instance().add(instrumentNameMangled, instr);
      }
      // Now set the instrument
      this->setInstrument(instr);
    }
  }


  //-------------------------------------------------------------------------------------------------
  /** Parse the result of ParameterMap.asString() into the ParameterMap
   * of the current instrument. The instrument needs to have been loaded
   * already, of course.
   *
   * @param parameterStr :: result of ParameterMap.asString()
   */
  void ExperimentInfo::readParameterMap(const std::string & parameterStr)
  {
    Geometry::ParameterMap& pmap = this->instrumentParameters();
    Instrument_const_sptr instr = this->getInstrument()->baseInstrument();

    int options = Poco::StringTokenizer::TOK_IGNORE_EMPTY;
    options += Poco::StringTokenizer::TOK_TRIM;
    Poco::StringTokenizer splitter(parameterStr, "|", options);

    Poco::StringTokenizer::Iterator iend = splitter.end();
    //std::string prev_name;
    for( Poco::StringTokenizer::Iterator itr = splitter.begin(); itr != iend; ++itr )
    {
      Poco::StringTokenizer tokens(*itr, ";");
      if( tokens.count() != 4 ) continue;
      std::string comp_name = tokens[0];
      //if( comp_name == prev_name ) continue; this blocks reading in different parameters of the same component. RNT
      //prev_name = comp_name;
      const Geometry::IComponent* comp = 0;
      if (comp_name.find("detID:") != std::string::npos)
      {
        int detID = atoi(comp_name.substr(6).c_str());
        comp = instr->getDetector(detID).get();
        if (!comp)
        {
          g_log.warning()<<"Cannot find detector "<<detID<<'\n';
          continue;
        }
      }
      else
      {
        comp = instr->getComponentByName(comp_name).get();
        if (!comp)
        {
          g_log.warning()<<"Cannot find component "<<comp_name<<'\n';
          continue;
        }
      }
      if( !comp ) continue;
      pmap.add(tokens[1], comp, tokens[2], tokens[3]);
    }
  }

//  //--------------------------------------------------------------------------------------------
//  /** Save the reference to the instrument to an open NeXus file.
//   * @param file :: open NeXus file
//   * @param group :: name of the group to create
//   */
//  void ExperimentInfo::saveInstrumentNexus(::NeXus::File * file) const
//  {
//    file->makeGroup("instrument", "NXinstrument", 1);
//    file->putAttr("version", 1);
//    file->closeGroup();
//  }
//
//  //--------------------------------------------------------------------------------------------
//  /** Load the object from an open NeXus file.
//   * @param file :: open NeXus file
//   * @param group :: name of the group to open
//   */
//  void ExperimentInfo::loadInstrumentNexus(::NeXus::File * file)
//  {
//    file->openGroup("instrument", "NXinstrument");
//    file->closeGroup();
//  }




} // namespace Mantid
} // namespace API


namespace Mantid
{
  namespace Kernel
  {

    template<> MANTID_API_DLL
      Mantid::API::ExperimentInfo_sptr IPropertyManager::getValue<Mantid::API::ExperimentInfo_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::API::ExperimentInfo_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::API::ExperimentInfo_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected ExperimentInfo.";
        throw std::runtime_error(message);
      }
    }

    template<> MANTID_API_DLL
      Mantid::API::ExperimentInfo_const_sptr IPropertyManager::getValue<Mantid::API::ExperimentInfo_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::API::ExperimentInfo_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::API::ExperimentInfo_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const ExperimentInfo.";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid
