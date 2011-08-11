#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <map>

using namespace Mantid::Geometry;

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
  void ExperimentInfo::setInstrument(const IInstrument_sptr& instr)
  {
    boost::shared_ptr<Instrument> tmp = boost::dynamic_pointer_cast<Instrument>(instr);
    if (tmp->isParametrized())
    {
      sptr_instrument = tmp->baseInstrument();
      m_parmap = tmp->getParameterMap();
    }
    else
    {
      sptr_instrument=tmp;
    }
  }


  //---------------------------------------------------------------------------------------
  /** Get a shared pointer to the parametrized instrument associated with this workspace
  *
  *  @return The instrument class
  */
  IInstrument_sptr ExperimentInfo::getInstrument()const
  {
    return Geometry::ParComponentFactory::createInstrument(sptr_instrument, m_parmap);
  }

  //---------------------------------------------------------------------------------------
  /** Get a shared pointer to the base instrument (i.e. non-parameterized) associated with this workspace
  *getInstrument
  *  @return The instrument class
  */
  boost::shared_ptr<Instrument> ExperimentInfo::getBaseInstrument() const
  {
    return sptr_instrument;
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

} // namespace Mantid
} // namespace API

