// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
#include <boost/variant.hpp>
#include <memory>

// Poco
#include <Poco/DOM/DOMParser.h>
#include <Poco/XML/XML.h>
