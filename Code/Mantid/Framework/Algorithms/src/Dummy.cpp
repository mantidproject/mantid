#include "MantidAlgorithms/Dummy.h"
#include "MantidKernel/System.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MultipleFileProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(Dummy)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Dummy::Dummy()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Dummy::~Dummy()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void Dummy::initDocs()
  {
    this->setOptionalMessage("Dummy algorithm for testing");

  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void Dummy::init()
  {
    declareProperty( new FileProperty("File", "", FileProperty::Load));
    declareProperty( new MultipleFileProperty("ManyFiles"));

    declareProperty( new WorkspaceProperty<>("InputWorkspace", "", Direction::Input) );

    declareProperty("IntProp1", 123);
    declareProperty("EnabledWhenDefault", 123);
    setPropertySettings("EnabledWhenDefault", new EnabledWhenProperty(this, "IntProp1", IS_DEFAULT) );

    declareProperty("BoolProp1", false);
    declareProperty("EnabledWhenNotDefault", 123);
    setPropertySettings("EnabledWhenNotDefault", new EnabledWhenProperty(this, "BoolProp1", IS_NOT_DEFAULT) );

    // Secret property!
    declareProperty("BoolProp2", false);
    declareProperty("InvisibleProp", 123);
    setPropertySettings("InvisibleProp", new VisibleWhenProperty(this, "BoolProp2", IS_EQUAL_TO, "1"));
    declareProperty( new WorkspaceProperty<>("InvisibleWorkspace", "", Direction::Output) );
    setPropertySettings("InvisibleWorkspace", new VisibleWhenProperty(this, "BoolProp2", IS_EQUAL_TO, "1") );

    std::vector<std::string> propOptions;
    propOptions.push_back("Q (lab frame)");
    propOptions.push_back("Q (sample frame)");
    propOptions.push_back("HKL");
    declareProperty("OutputDimensions", "Q (lab frame)", new ListValidator(propOptions));

    IPropertySettings * set =  new VisibleWhenProperty(this, "OutputDimensions", IS_EQUAL_TO, "HKL");
    declareProperty("InvisibleProp2", 123);
    setPropertySettings("InvisibleProp2", set);
    declareProperty( new FileProperty("File2", "", FileProperty::Load));
    setPropertySettings("File2", set->clone() );


  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void Dummy::exec()
  {
  }



} // namespace Mantid
} // namespace Algorithms

