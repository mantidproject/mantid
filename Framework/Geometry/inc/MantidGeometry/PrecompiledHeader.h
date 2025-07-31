// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// Mantid
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/V3D.h"

// STL
#include <cfloat>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

// Boost
#include <boost/algorithm/string.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/multi_array.hpp>
#include <boost/unordered_set.hpp>
#include <memory>

// Poco
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
