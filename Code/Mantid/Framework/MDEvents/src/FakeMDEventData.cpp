#include "MantidMDEvents/FakeMDEventData.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ArrayProperty.h"

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
    this->setWikiSummary("Adds fake multi-dimensional event data to a MDEventWorkspace, for use in testing.");
    this->setOptionalMessage("Adds fake multi-dimensional event data to a MDEventWorkspace, for use in testing.");
  }


  //----------------------------------------------------------------------------------------------
  /** Function makes up a fake single-crystal peak and adds it to the workspace.
   *
   * @param ws
   */
  template<typename MDE, size_t nd>
  void FakeMDEventData::addFakePeak(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    std::cout << "Workspace " << ws->getName() << " has " << nd << " dims\n";
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
        "Parameters: x, y, z, ..., width.\n");

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
//
//    CALL_MDEVENT_FUNCTION(this->addFakePeak, in_ws)
//    setProperty("OutputWorkspace", in_ws);
  }



} // namespace Mantid
} // namespace MDEvents

