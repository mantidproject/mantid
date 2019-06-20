// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/StdoutChannel.h"
#include <iostream>

namespace Poco {
StdoutChannel::StdoutChannel() : ConsoleChannel(std::cout) {}
} // namespace Poco
