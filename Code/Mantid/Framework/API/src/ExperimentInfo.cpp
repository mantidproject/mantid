#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <map>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace API
{


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
   * @param other :: the source from which to copy ExperimentInfo */
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
  void ExperimentInfo::setInstrument(const Instrument_sptr& instr)
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
  Instrument_sptr ExperimentInfo::getInstrument()const
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
    boost::shared_ptr<const Instrument> instrument = getInstrument()->baseInstrument();//getBaseInstrument();
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


  //--------------------------------------------------------------------------------------------
  /** Save the object to an open NeXus file.
   * @param file :: open NeXus file
   */
  void ExperimentInfo::saveExperimentInfoNexus(::NeXus::File * file) const
  {
    m_sample->saveNexus(file, "sample");
    m_run->saveNexus(file, "logs");
    // TODO: Parameter map, instrument.
  }

  //--------------------------------------------------------------------------------------------
  /** Load the object from an open NeXus file.
   * @param file :: open NeXus file
   */
  void ExperimentInfo::loadExperimentInfoNexus(::NeXus::File * file)
  {
    m_sample.access().loadNexus(file, "sample");
    m_run.access().loadNexus(file, "logs");
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
