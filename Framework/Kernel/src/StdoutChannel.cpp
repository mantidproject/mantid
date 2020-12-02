// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/StdoutChannel.h"
#include <iostream>
#include <Poco/Message.h>

#include <iosfwd>                          // streamsize
#include <boost/iostreams/categories.hpp>  // sink_tag

namespace Poco {
StdoutChannel::StdoutChannel() : ConsoleChannel(std::cout) {std::cout << "[DEBUG]...............  Am I called??";}


/// overwrite log
void StdoutChannel::log(const Message& msg) {
    std::cout << "[...... OVERRIDE ] " << msg.getText()  << "\n";
}


} // namespace Poco
