#ifndef MANTID_API_PRECOMPILED_HEADER_H_
#define MANTID_API_PRECOMPILED_HEADER_H_

// Mantid
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

// STL
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

// Poco
#include <Poco/DOM/DOMParser.h>
#include <Poco/XML/XML.h>

#endif // MANTID_API_PRECOMPILED_HEADER_H_
