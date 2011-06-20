#ifndef MANTID_API_EXPERIMENTINFO_H_
#define MANTID_API_EXPERIMENTINFO_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/IInstrument.h"


namespace Mantid
{
namespace API
{

  /** This class is shared by a few Workspace types
   * and holds information related to a particular experiment/run:
   *
   * - Instrument (with parameter map)
   * - Run object (sample logs)
   * - Sample object (sample info)
   * 
   * @author Janik Zikovsky
   * @date 2011-06-20
   */
  class DLLExport ExperimentInfo 
  {
  public:
    ExperimentInfo();
    ~ExperimentInfo();
    
    void copyExperimentInfoFrom(ExperimentInfo * other);

    /// Instrument accessors
    void setInstrument(const Geometry::IInstrument_sptr&);
    Geometry::IInstrument_sptr getInstrument() const;
    boost::shared_ptr<Geometry::Instrument> getBaseInstrument() const;

    /// Returns the set of parameters modifying the base instrument
    const Geometry::ParameterMap& instrumentParameters() const;
    Geometry::ParameterMap& instrumentParameters();
    /// Const version
    const Geometry::ParameterMap& constInstrumentParameters() const;
    // Add parameters to the instrument parameter map
    void populateInstrumentParameters();

    /// Sample accessors
    const Sample& sample() const;
    /// Writable version of the sample object
    Sample& mutableSample();

    /// Run details object access
    const Run & run() const;
    /// Writable version of the run object
    Run& mutableRun();


  protected:
    /// The information on the sample environment
    Kernel::cow_ptr<Sample> m_sample;
    /// The run information
    Kernel::cow_ptr<Run> m_run;
    /// Parameters modifying the base instrument
    boost::shared_ptr<Geometry::ParameterMap> m_parmap;
    /// The base (unparametrized) instrument
    boost::shared_ptr<Geometry::Instrument> sptr_instrument;

  };


} // namespace Mantid
} // namespace API

#endif  /* MANTID_API_EXPERIMENTINFO_H_ */
