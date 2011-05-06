#include "MantidMDEvents/FakeMDEventData.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/math/distributions/normal.hpp>

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(FakeMDEventData)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FakeMDEventData::FakeMDEventData()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FakeMDEventData::~FakeMDEventData()
  {
  }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void FakeMDEventData::initDocs()
  {
    this->setWikiSummary("Adds fake multi-dimensional event data to an existing MDEventWorkspace, for use in testing.\nYou can create a blank MDEventWorkspace with CreateMDEventWorkspace.");
    this->setOptionalMessage("Adds fake multi-dimensional event data to an existing MDEventWorkspace, for use in testing.\nYou can create a blank MDEventWorkspace with CreateMDEventWorkspace.");
  }


  //----------------------------------------------------------------------------------------------
  /** Function makes up a fake single-crystal peak and adds it to the workspace.
   *
   * @param ws
   */
  template<typename MDE, size_t nd>
  void FakeMDEventData::addFakePeak(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::vector<double> params = getProperty("PeakParams");
    if (params.size() == 0)
      return;

    if (params.size() != nd + 2)
      throw std::invalid_argument("PeakParams needs to have ndims+2 arguments.");
    if (params[0] <= 0)
      throw std::invalid_argument("PeakParams: number_of_events needs to be > 0");
    size_t num = size_t(params[0]);

    // Width of the peak
    double width = params.back();

    boost::mt19937 rng;
    boost::uniform_real<double> u(-width, width); // Range
//    boost::math::normal_distribution<double> u(0, width);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<double> > gen(rng, u);

    for (size_t i=0; i<num; ++i)
    {
      CoordType centers[nd];
      for (size_t d=0; d<nd; d++)
        centers[d] = params[d+1] + gen(); // Center + random variation
      // Create and add the event.
      ws->addEvent( MDE( 1.0, 1.0, centers) );
    }

    ws->splitBox();
    ws->splitAllIfNeeded(NULL);
    ws->refreshCache();
  }

  //----------------------------------------------------------------------------------------------
  /** Function makes up a fake uniform event data and adds it to the workspace.
   *
   * @param ws
   */
  template<typename MDE, size_t nd>
  void FakeMDEventData::addFakeUniformData(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::vector<double> params = getProperty("UniformParams");
    if (params.size() == 0)
      return;

    if (params[0] <= 0)
      throw std::invalid_argument("UniformParams: number_of_events needs to be > 0");
    size_t num = size_t(params[0]);

    if (params.size() == 1)
    {
      for (size_t d=0; d<nd; ++d)
      {
        params.push_back( ws->getDimension(d)->getMinimum() );
        params.push_back( ws->getDimension(d)->getMaximum() );
      }
    }
    if (params.size() != 1 + nd*2)
      throw std::invalid_argument("UniformParams: needs to have ndims*2+1 arguments.");

    boost::mt19937 rng;

    // Make a random generator for each dimensions
    typedef boost::variate_generator<boost::mt19937&, boost::uniform_real<double> >   gen_t;
    gen_t * gens[nd];
    for (size_t d=0; d<nd; ++d)
    {
      double min = params[d*2+1];
      double max = params[d*2+2];
      if (max <= min) throw std::invalid_argument("UniformParams: min must be < max for all dimensions.");
      boost::uniform_real<double> u(min,max); // Range
      gen_t * gen = new gen_t(rng, u);
      gens[d] = gen;
    }

    // Create all the requested events
    for (size_t i=0; i<num; ++i)
    {
      CoordType centers[nd];
      for (size_t d=0; d<nd; d++)
        centers[d] = (*gens[d])(); // use a different generator for each dimension
      // Create and add the event.
      ws->addEvent( MDE( 1.0, 1.0, centers) );
    }

    /// Clean up the generators
    for (size_t d=0; d<nd; ++d)
      delete gens[d];

    ws->splitBox();
    ws->splitAllIfNeeded(NULL);
    ws->refreshCache();
  }


  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void FakeMDEventData::init()
  {
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::InOut),
        "An input workspace, that will get MDEvents added to it");

    declareProperty(new ArrayProperty<double>("UniformParams", ""),
        "Add a uniform, randomized distribution of events.\n"
        "1 parameter: number_of_events; they will be distributed across the size of the workspace.\n"
        "Multiple parameters: number_of_events, min,max (for each dimension); distribute the events inside the range given.\n");

    declareProperty(new ArrayProperty<double>("PeakParams", ""),
        "Add a peak with a normal distribution around a central point.\n"
        "Parameters: number_of_events, x, y, z, ..., width.\n");

//    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::InOut),
//        "An input workspace, that will get MDEvents added to it");

//    std::vector<std::string> propOptions;
//    propOptions.push_back("Uniform");
//    propOptions.push_back("Peak");
//    declareProperty("DataType", "Uniform",new ListValidator(propOptions),
//      "Which underlying data type will event take (only one option is currently available).");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void FakeMDEventData::exec()
  {
    IMDEventWorkspace_sptr in_ws = getProperty("InputWorkspace");

    if (getPropertyValue("UniformParams")=="" && getPropertyValue("PeakParams")=="")
      throw std::invalid_argument("You must specify at least one of PeakParams or UniformParams.");

    CALL_MDEVENT_FUNCTION(this->addFakePeak, in_ws)
    CALL_MDEVENT_FUNCTION(this->addFakeUniformData, in_ws)
  }



} // namespace Mantid
} // namespace MDEvents

