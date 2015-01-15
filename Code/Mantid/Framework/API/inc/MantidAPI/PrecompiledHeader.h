#ifndef MANTID_API_PRECOMPILED_HEADER_H_
#define MANTID_API_PRECOMPILED_HEADER_H_

// Mantid
#include "MantidKernel/System.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/NearestNeighbours.h"

// STL
#include <vector>
#include <map>
#include <string>
#include <set>
#include <cmath>
#include <cfloat>
#include <cstddef>
#include <utility>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

// Poco
#include <Poco/XML/XML.h>
#include <Poco/DOM/DOMParser.h>

#endif // MANTID_API_PRECOMPILED_HEADER_H_
