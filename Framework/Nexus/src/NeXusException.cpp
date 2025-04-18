#include "MantidNexus/NeXusException.hpp"
#include "MantidNexus/NeXusFile_fwd.h"

/**
 * \file NeXusException.cpp
 * The implementation of the NeXus::Exception class
 */

namespace NeXus {

Exception::Exception(const std::string &msg, const std::string &filename)
    : std::runtime_error(msg), m_filename(filename) {}

const std::string Exception::filename() const throw() { return this->m_filename; }

} // namespace NeXus
