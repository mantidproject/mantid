#include "MantidCrystal/SetGoniometer.h"
#include "MantidKernel/System.h"
#include <boost/algorithm/string/split.hpp>
#include "MantidKernel/Strings.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include <boost/algorithm/string/detail/classification.hpp>
#include "MantidGeometry/V3D.h"

using Mantid::Geometry::Goniometer;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SetGoniometer)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  /// How many axes (max) to define
  const size_t NUM_AXES = 6;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SetGoniometer::SetGoniometer()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SetGoniometer::~SetGoniometer()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SetGoniometer::initDocs()
  {
    this->setWikiSummary("Define the goniometer motors used in an experiment by giving the axes and directions of rotations.");
    this->setOptionalMessage("Define the goniometer motors used in an experiment by giving the axes and directions of rotations.");
    this->setWikiDescription(""
        "Use this algorithm to define your goniometer. "
        "Enter each axis in the order of rotation, starting with the one closest to the sample. \n"
        "\n"
        "You may enter up to 6 axes, for which you must define (separated by commas): \n"
        "* The name of the axis, which MUST match the name in your sample logs.\n"
        "* The X, Y, Z components of the vector of the axis of rotation. Right-handed coordinates with +Z=beam direction; +Y=Vertically up (against gravity); +X to the left.\n"
        "* The sense of rotation as 1 or -1: 1 for counter-clockwise, -1 for clockwise rotation.\n"
        "\n"
        "The run's sample logs will be used in order to determine the actual angles of rotation: "
        "for example, if you have an axis called 'phi', then the first value of the log called "
        "'phi' will be used as the rotation angle. Units are assumed to be degrees.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SetGoniometer::init()
  {
    declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut), "An workspace that will be modified with the new goniometer created.");

    std::string axisHelp = ": name, x,y,z, 1/-1 (1 for ccw, -1 for cw rotation). Leave blank for no axis";
    for (size_t i=0; i< NUM_AXES; i++)
    {
      std::ostringstream propName;
      propName << "Axis" << i;
      declareProperty(new PropertyWithValue<std::string>(propName.str(),"",Direction::Input), propName.str() + axisHelp);
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SetGoniometer::exec()
  {
    MatrixWorkspace_sptr ws = getProperty("Workspace");
    // Create the goniometer
    Goniometer gon;

    for (size_t i=0; i< NUM_AXES; i++)
    {
      std::ostringstream propName;
      propName << "Axis" << i;
      std::string axisDesc = getPropertyValue(propName.str());

      if (!axisDesc.empty())
      {
        std::vector<std::string> tokens;
        boost::split( tokens, axisDesc, boost::algorithm::detail::is_any_ofF<char>(","));
        if (tokens.size() != 5)
          throw std::invalid_argument("Wrong number of arguments to parameter " + propName.str() + ". Expected 5 comma-separated arguments.");

        std::string axisName = tokens[0];
        axisName = Strings::strip(axisName);
        if (axisName.empty())
          throw std::invalid_argument("The name must not be empty");

        double x=0,y=0,z=0;
        if (!Strings::convert(tokens[1], x)) throw std::invalid_argument("Error converting string '" + tokens[1] + "' to a number.");
        if (!Strings::convert(tokens[2], y)) throw std::invalid_argument("Error converting string '" + tokens[2] + "' to a number.");
        if (!Strings::convert(tokens[3], z)) throw std::invalid_argument("Error converting string '" + tokens[3] + "' to a number.");
        V3D vec(x,y,z);
        if (vec.norm() < 1e-4)
          throw std::invalid_argument("Rotation axis vector should be non-zero!");

        int ccw = 0;
        Strings::convert(tokens[4], ccw);
        if (ccw != 1 && ccw != -1)
          throw std::invalid_argument("The ccw parameter must be 1 (ccw) or -1 (cw) but no other value.");
        // Default to degrees
        gon.pushAxis( axisName, x, y, z, 0.0, ccw);
      }
    }

    if (gon.getNumberAxes() == 0)
      throw std::invalid_argument("No axes defined!");

    // All went well, copy the goniometer into it
    ws->mutableRun().getGoniometer() = gon;

  }



} // namespace Mantid
} // namespace Crystal

