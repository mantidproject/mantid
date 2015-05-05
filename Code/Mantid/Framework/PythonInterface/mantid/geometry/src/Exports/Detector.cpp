#include "MantidGeometry/Instrument/Detector.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::Detector;
using Mantid::Geometry::IDetector;
using Mantid::Geometry::ObjComponent;
using namespace boost::python;

/**
 * Enables boost.python to automatically "cast" an object up to the
 * appropriate Detector leaf type 
 */
// clang-format off
void export_Detector()
// clang-format on
{
  class_<Detector, bases<IDetector, ObjComponent>, boost::noncopyable>("Detector", no_init)
    ;
}

