// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/StdoutChannel.h"
#include <Poco/Message.h>
#include <iostream>

#include <boost/iostreams/categories.hpp> // sink_tag
#include <iosfwd>                         // streamsize

namespace Poco {
StdoutChannel::StdoutChannel() : ConsoleChannel(std::cout) {}

} // namespace Poco
