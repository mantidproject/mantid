#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"
#include "MantidNexusCpp/napiconfig.h"

/**
 * \file NeXusException.cpp
 * The implementation of the NeXus::Exception class
 */

namespace NeXus {

Exception::Exception(const std::string &msg, const int status) : std::runtime_error(msg) {
  this->m_what = msg;
  this->m_status = status;
}

const char *Exception::what() const throw() { return this->m_what.c_str(); }

int Exception::status() throw() { return this->m_status; }

Exception::~Exception() throw() {}

} // namespace NeXus
