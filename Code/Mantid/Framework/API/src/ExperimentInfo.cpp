#include "MantidAPI/ExperimentInfo.h"

#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ModeratorModel.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"

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

#include <boost/regex.hpp>

#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/SAXParser.h>

#include <fstream>
#include <map>

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
  :
    m_moderatorModel(),
    m_choppers(),
    m_sample(),
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
    if(other->m_moderatorModel) m_moderatorModel = other->m_moderatorModel->clone();
    m_choppers.clear();
    for(auto iter = other->m_choppers.begin(); iter != other->m_choppers.end(); ++iter)
    {
      m_choppers.push_back((*iter)->clone());
    }
  }

  //---------------------------------------------------------------------------------------
  /** Clone this ExperimentInfo class into a new one
   */
  ExperimentInfo * ExperimentInfo::cloneExperimentInfo()const
  {
    ExperimentInfo * out = new ExperimentInfo();
    out->copyExperimentInfoFrom(this);
    return out;
  }


  //---------------------------------------------------------------------------------------
  /** Set the instrument
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

  /** HACK -- the internal function which allows to interpret string values found in IDF parameters file as boolean 
   *  it is hack as this funtion should be somewhere else. 
   *
   *@param boolType -- the string representation of the boolean value one wants to compare. 
   *@return  True if the name within of the list of true names, and false otherwise
  */
  const char * TrueNames[2 ]={"True","Yes"};
  bool convertBool(const std::string &boolType)
  {
      for(int i=0;i<2;i++)
      {
          if(std::strcmp(boolType.c_str(),TrueNames[i])==0)return true;
      }
      return false;
  }

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

      bool is_int_val =  (category.compare("int") == 0);
      bool is_string_val =  (category.compare("string") == 0);
      bool is_bool_val  =  (category.compare("bool") == 0);
      bool is_double_val = !(is_string_val || is_bool_val || is_int_val);

      // if category is not double sting no point in trying to generate a double from parameter
      double value = 0.0;
      if ( is_double_val)
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
      else if (is_string_val)
      {
        paramMap.addString(((*it).second)->m_component, paramN, ((*it).second)->m_value);
      }
      else if (is_bool_val )
      {
          bool b_val = convertBool(((*it).second)->m_value);
          paramMap.addBool(((*it).second)->m_component,paramN,b_val);
      }
      else if (is_int_val )
      {
          int i_val = boost::lexical_cast<int>(((*it).second)->m_value);
          paramMap.addInt(((*it).second)->m_component,paramN,i_val);
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
  /**
   * Replaces current parameter map with a copy of the given map
   * @ pmap const reference to parameter map whose copy replaces the current parameter map
   */
  void ExperimentInfo::replaceInstrumentParameters(const Geometry::ParameterMap & pmap)
  {
    this->m_parmap.reset(new ParameterMap(pmap));
  }

  //---------------------------------------------------------------------------------------
  /**
   * Caches a lookup for the detector IDs of the members that are part of the same group
   * @param mapping :: A map between a detector ID and the other IDs that are part of the same
   * group.
   */
  void ExperimentInfo::cacheDetectorGroupings(const det2group_map & mapping)
  {
    m_detgroups = mapping;
  }

  //---------------------------------------------------------------------------------------
  /// Returns the detector IDs that make up the group that this ID is part of
  const std::vector<detid_t> & ExperimentInfo::getGroupMembers(const detid_t detID) const
  {
    auto iter = m_detgroups.find(detID);
    if(iter != m_detgroups.end())
    {
      return iter->second;
    }
    else
    {
      throw std::runtime_error("ExperimentInfo::getGroupMembers - Unable to find ID " + boost::lexical_cast<std::string>(detID) + " in lookup");
    }
  }

  //---------------------------------------------------------------------------------------
  /**
   * Get a detector or detector group from an ID
   * @param detID :: 
   * @returns A single detector or detector group depending on the mapping set.
   * @see set
   */
  Geometry::IDetector_const_sptr ExperimentInfo::getDetectorByID(const detid_t detID) const
  {
    if(m_detgroups.empty())
    {
      g_log.debug("No detector mapping cached, getting detector from instrument");
      return getInstrument()->getDetector(detID);
    }
    else
    {
      const std::vector<detid_t> & ids = this->getGroupMembers(detID);
      return getInstrument()->getDetectorG(ids);
    }
  }


  //---------------------------------------------------------------------------------------

  /**
   * Set an object describing the moderator properties and take ownership
   * @param source :: A pointer to an object describing the source. Ownership is transferred to this object
   */
  void ExperimentInfo::setModeratorModel(ModeratorModel *source)
  {
    if(!source)
    {
      throw std::invalid_argument("ExperimentInfo::setModeratorModel - NULL source object found.");
    }
    m_moderatorModel = boost::shared_ptr<ModeratorModel>(source);
  }

  /// Returns a reference to the source properties object
  ModeratorModel & ExperimentInfo::moderatorModel() const
  {
    if(!m_moderatorModel)
    {
      throw std::runtime_error("ExperimentInfo::moderatorModel - No source desciption has been defined");
    }
    return *m_moderatorModel;
  }

  //---------------------------------------------------------------------------------------
  /**
   * Sets a new chopper description at a given point. The point is given by index where 0 is
   * closest to the source
   * @param chopper :: A pointer to a new chopper object, this class takes ownership of the pointer
   * @param index :: An optional index that specifies which chopper point the chopper belongs to (default=0)
   */
  void ExperimentInfo::setChopperModel(ChopperModel *chopper, const size_t index)
  {
    if(!chopper)
    {
      throw std::invalid_argument("ExperimentInfo::setChopper - NULL chopper object found.");
    }
    auto iter = m_choppers.begin();
    std::advance(iter, index);
    if(index < m_choppers.size()) // Replacement
    {
      (*iter) = boost::shared_ptr<ChopperModel>(chopper);
    }
    else // Insert it
    {
      m_choppers.insert(iter, boost::shared_ptr<ChopperModel>(chopper));
    }
  }

  /**
   * Returns a const reference to a chopper description
   * @param index :: An optional index giving the point within the instrument this chopper describes (default=0)
   * @return A reference to a const chopper object
   */
  ChopperModel & ExperimentInfo::chopperModel(const size_t index) const
  {
    if(index < m_choppers.size())
    {
      auto iter = m_choppers.begin();
      std::advance(iter, index);
      return **iter;
    }
    else
    {
      std::ostringstream os;
      os << "ExperimentInfo::chopper - Invalid index=" << index << ". " << m_choppers.size()
         << " chopper descriptions have been set.";
      throw std::invalid_argument(os.str());
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

  /**
   * Get an experimental log either by log name or by type, e.g.
   *   - temperature_log
   *   - chopper_speed_log
   * The logs are first checked for one matching the given string and if that
   * fails then the instrument is checked for a parameter of the same name
   * and if this exists then its value is assume to be the actual log required
   * @param log :: A string giving either a specific log name or instrument parameter whose
   * value is to be retrieved
   * @return A pointer to the property
   */
  Kernel::Property * ExperimentInfo::getLog(const std::string & log) const
  {
    try
    {
      return run().getProperty(log);
    }
    catch(Kernel::Exception::NotFoundError&)
    {
      // No log with that name
    }
    // If the instrument has a parameter with that name then take the value as a log name
    const std::string logName = instrumentParameters().getString(sptr_instrument.get(), log);
    if(logName.empty())
    {
      throw std::invalid_argument("ExperimentInfo::getLog - No instrument parameter named \""
         + log + "\". Cannot access full log name");
    }
    return run().getProperty(logName);
  }

  /**
   * Get an experimental log as a single value either by log name or by type. @see getLog
   * @param log :: A string giving either a specific log name or instrument parameter whose
   * value is to be retrieved
   * @return A pointer to the property
   */
  double ExperimentInfo::getLogAsSingleValue(const std::string & log) const
  {
    try
    {
      return run().getPropertyAsSingleValue(log);
    }
    catch(Kernel::Exception::NotFoundError&)
    {
      // No log with that name
    }
    // If the instrument has a parameter with that name then take the value as a log name
    const std::string logName = instrumentParameters().getString(sptr_instrument.get(), log);
    if(logName.empty())
    {
      throw std::invalid_argument("ExperimentInfo::getLog - No instrument parameter named \""
         + log + "\". Cannot access full log name");
    }
    return run().getPropertyAsSingleValue(logName);
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

  /**
   * Returns the emode for this run. It first searchs the run logs for a "deltaE-mode" log and falls back to
   * the instrument if one is not found. If neither exist then the run is considered Elastic.
   * @return The emode enum for the energy transfer mode of this run. Currently only checks the instrument
   */
  Kernel::DeltaEMode::Type ExperimentInfo::getEMode() const
  {
    static const char * emodeTag = "deltaE-mode";
    std::string emodeStr;
    if(run().hasProperty(emodeTag))
    {
      emodeStr = run().getPropertyValueAsType<std::string>(emodeTag);
    }
    else if(sptr_instrument && instrumentParameters().contains(sptr_instrument.get(), emodeTag))
    {
      Geometry::Parameter_sptr param = instrumentParameters().get(sptr_instrument.get(), emodeTag);
      emodeStr = param->asString();
    }
    else
    {
      return Kernel::DeltaEMode::Elastic;
    }
    return Kernel::DeltaEMode::fromString(emodeStr);
  }

  /**
   * Easy access to the efixed value for this run & detector ID
   * @param detID :: The detector ID to ask for the efixed mode (ignored in Direct & Elastic mode). The
   * detector with ID matching that given is pulled from the instrument with this method and it will
   * throw a Exception::NotFoundError if the ID is unknown.
   * @return The current EFixed value
   */
  double ExperimentInfo::getEFixed(const detid_t detID) const
  {
    IDetector_const_sptr det = getInstrument()->getDetector(detID);
    return getEFixed(det);
  }

  /**
   * Easy access to the efixed value for this run & detector
   * @param detector :: The detector object to ask for the efixed mode. Only required for Indirect mode
   * @return The current efixed value
   */
  double ExperimentInfo::getEFixed(const Geometry::IDetector_const_sptr detector) const
  {
    Kernel::DeltaEMode::Type emode = getEMode();
    if(emode == Kernel::DeltaEMode::Direct)
    {
      try
      {
        return this->run().getPropertyValueAsType<double>("Ei");
      }
      catch(Kernel::Exception::NotFoundError &)
      {
        throw std::runtime_error("Experiment logs do not contain an Ei value. Have you run GetEi?");
      }
    }
    else if(emode == Kernel::DeltaEMode::Indirect)
    {
      if(!detector) throw std::runtime_error("ExperimentInfo::getEFixed - Indirect mode efixed requested without a valid detector.");
      Parameter_sptr par = constInstrumentParameters().getRecursive(detector.get(),"Efixed");
      if (par)
      {
        return par->value<double>();
      }
      else
      {
        std::ostringstream os;
        os << "ExperimentInfo::getEFixed - Indirect mode efixed requested but detector has no Efixed parameter attached. ID=" << detector->getID();
        throw std::runtime_error(os.str());
      }
    }
    else
    {
      throw std::runtime_error("ExperimentInfo::getEFixed - EFixed requested for elastic mode, don't know what to do!");
    }
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
    try
    {
      date = m_run->startTime().toISO8601String();
    }
    catch (std::runtime_error &)
    {
      g_log.information("run_start/start_time not stored in workspace. Default to current date.");
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
      if (date.empty())
      {
          // Just use the current date
          g_log.debug() << "No date specified, using current date and time." << std::endl;
          const std::string now = Kernel::DateAndTime::getCurrentTime().toISO8601String();
          // Recursively call this method, but with both parameters.
          return ExperimentInfo::getInstrumentFilename(instrumentName, now);
      }

    g_log.debug() << "Looking for instrument XML file for " << instrumentName << " that is valid on '" << date << "'\n";
    // Lookup the instrument (long) name
    std::string instrument(Kernel::ConfigService::Instance().getInstrument(instrumentName).name());

    // Get the search directory for XML instrument definition files (IDFs)
    std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();

    boost::regex regex(instrument+"_Definition.*\\.xml", boost::regex_constants::icase);
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
      if ( regex_match(l_filenamePart, regex) )
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
          to.setFromISO8601("2100-01-01T00:00:00");

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
    g_log.debug() << "IDF selected is " << mostRecentIDF << std::endl;
    return mostRecentIDF;
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

    // We first assume this is a new version file, but if the next step fails we assume its and old version file.
    int version = 1;
    try {
      file->readData("instrument_source", instrumentFilename);
    } 
    catch(...) {
      version = 0;
      file->closeGroup();
      file->readData("instrument_source", instrumentFilename);
    }

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
      if( tokens.count() < 4 ) continue;
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
      // create parameter's value as a sum of all tokens with index 3 or larger
      // this allow a parameter's value to contain ";" 
      std::string paramValue = tokens[3];
      int size = static_cast<int>(tokens.count());
      for (int i = 4; i < size; i++ )
        paramValue += ";" + tokens[4];
      pmap.add(tokens[1], comp, tokens[2], paramValue);
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
