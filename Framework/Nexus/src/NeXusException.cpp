#include "MantidNexus/NeXusException.hpp"
#include "MantidNexus/NeXusFile_fwd.h"

/**
 * \file NeXusException.cpp
 * The implementation of the NeXus::Exception class
 */

namespace NeXus {

Exception::Exception(const std::string &msg, const NXstatus status)
    : std::runtime_error(msg), m_what(msg), m_status(status) {}

const char *Exception::what() const throw() { return this->m_what.c_str(); }

NXstatus Exception::status() const throw() { return this->m_status; }

Exception::~Exception() throw() {}

} // namespace NeXus
