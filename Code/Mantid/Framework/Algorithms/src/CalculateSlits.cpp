#include "MantidAlgorithms/CalculateSlits.h"

#include <boost/shared_ptr.hpp>
#include <math.h>

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateSlits)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
CalculateSlits::CalculateSlits() {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
CalculateSlits::~CalculateSlits() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's name for identification. @see Algorithm::name
const std::string CalculateSlits::name() const {
  return "CalculateSlits";
}

/// Algorithm's version for identification. @see Algorithm::version
int CalculateSlits::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CalculateSlits::category() const {
  return "Reflectometry\\ISIS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateSlits::summary() const {
  return "Calculates appropriate slit widths for reflectometry instruments.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void CalculateSlits::init() {
  declareProperty("Slit1Slit2", Mantid::EMPTY_DBL(),
      "Distance between slit 1 and slit 2 in mm");
  declareProperty("Slit2SA", Mantid::EMPTY_DBL(), "Offset in the beam direction in mm");
  declareProperty("Resolution", Mantid::EMPTY_DBL(), "Resolution");
  declareProperty("Footprint", Mantid::EMPTY_DBL(), "Footprint in mm");
  declareProperty("Angle", Mantid::EMPTY_DBL(), "Angle in degrees");

  declareProperty("Slit1", Mantid::EMPTY_DBL(), "Slit 1 width in mm",
      Direction::Output);
  declareProperty("Slit2", Mantid::EMPTY_DBL(), "Slit 2 width in mm",
      Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void CalculateSlits::exec() {
  const double res = getProperty("Resolution");
  const double fp = getProperty("Footprint");
  const double angleDeg = getProperty("Angle");
  const double s1s2 = getProperty("Slit1Slit2");
  const double s2sa = getProperty("Slit2SA");


  /*
       |←----d-----→|
                   _  _
   _    _       _-¯ | ↑
   ↑   | ¯-_ _-¯    | |
   S₂  | (Θ_X_Θ)    | S₁  ←---beam---
   ↓   |_-¯   ¯-_   | |
   ¯             ¯-_| ↓
                      ¯
                   _  _
                _-¯ | ↑
             _-¯    | |
          _-¯      _| | ½S₀
       _-¯α)      | | ↓
        ¯¯¯¯¯¯¯¯¯¯¯¯  ¯
       |←----d-----→|

   For the purposes of these diagrams, Θ has
   already been multiplied by the resolution.

   α = ½Θ
   t = tan(α)
   r = resolution
   f = footprint (???)
   u = unknown dimension

  S₀ = S₁ + S₂
     = 2•d•t

  S₁ = 2•d•t - S₂
     = 2•d•t - f•sin(α/r) + 2•u•t
     = 2•(d+u)•t - f•sin(α/r)

  S₂ = f•sin(α/r) - 2•u•t

  sin(α/r) is opp/hyp of the full angle, without the resolution coefficient
  if f is the hypotenuse of a triangle constructed from the full angle
  then f•sin(α/r) is the length of the side opposite the angle
  */

  //Convert angle to radians for our calculations
  const double a = angleDeg * M_PI / 180.0;

  const double s2 = (fp * sin(a)) - (2 * s2sa * tan(res * a));
  const double s1 = (2 * s1s2 * tan(res * a)) - s2;

  setProperty("Slit1", s1);
  setProperty("Slit2", s2);
}

} // namespace Algorithms
} // namespace Mantid
