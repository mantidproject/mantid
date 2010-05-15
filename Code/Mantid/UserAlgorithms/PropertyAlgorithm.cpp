#include "PropertyAlgorithm.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM(PropertyAlgorithm)

using namespace Kernel;

/**  Initialization code
 *
 *   Properties have to be declared here before they can be used
*/
void PropertyAlgorithm::init()
{
    // Declare simple properties by giving it a name and initial value
    // Property's type is determined by the type of the initial value.
    // Allowed types are: int, double, bool, and std::string
    declareProperty("IntValue",0);
    declareProperty("DoubleValue",0.01);
    declareProperty("BoolValue",false);
    declareProperty("StringValue","Empty");

    // Property names must be unique. Multiple declarations will cause a run time error
    //declareProperty("IntValue",1);

    // A validator puts restrictions on property's possible values
    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("PositiveIntValue",0, mustBePositive);

    // A validator belongs to the propery. Two properties cannot have the same validator.
    // To use the same validating conditions a validator can be cloned:
    declareProperty("PositiveIntValue1",0, mustBePositive->clone());

    // A property can be an array of int, double, or std::string
    declareProperty(new ArrayProperty<int>("IntArray"));
    declareProperty(new ArrayProperty<double>("DoubleArray"));
    declareProperty(new ArrayProperty<std::string>("StringArray"));

}

/** Executes the algorithm
 */
void PropertyAlgorithm::exec()
{
  	// g_log is a reference to the logger. It is used to print out information,
		// warning, and error messages  
		g_log.information() << "Running algorithm " << name() << " version " << version() << std::endl;

    // Retrieve properties values

    // getProperty returns numerical value
    int intValue = getProperty("IntValue");
    double doubleValue = getProperty("DoubleValue");
    bool boolValue = getProperty("BoolValue");
    std::string stringValue = getProperty("StringValue");

    // getPropertyValue returns string representation of the property value
    std::string doubleValueString = getPropertyValue("DoubleValue");

    g_log.information() << "IntValue    = " << intValue << std::endl;
    g_log.information() << "DoubleValue = " << doubleValue << ' ' << doubleValueString << std::endl;
    g_log.information() << "BoolValue   = " << boolValue << std::endl;
    g_log.information() << "StringValue = " << stringValue << std::endl;

    int positiveIntValue = getProperty("PositiveIntValue");
    g_log.information() << "PositiveIntValue    = " << positiveIntValue << std::endl;

    std::vector<int> intArray = getProperty("IntArray");
    g_log.information() << "Size of IntArray    = " << intArray.size() << std::endl;

    std::vector<double> doubleArray = getProperty("DoubleArray");
    g_log.information() << "Size of DoubleArray = " << doubleArray.size() << std::endl;

    std::vector<std::string> stringArray = getProperty("StringArray");
    g_log.information() << "Size of StringArray = " << stringArray.size() << std::endl;

}

}
}

