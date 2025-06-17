#include "MantidNexus/NexusException.h"
#include <ostream>

/**
 * The implementation of the Nexus::Exception class
 */

namespace Mantid::Nexus {

Exception::Exception(const std::string &msg, const std::string &functionname, const std::string &filename)
    : std::runtime_error(msg), m_functionname(functionname), m_filename(filename) {}

const std::string Exception::functionname() const throw() { return this->m_functionname; }

const std::string Exception::filename() const throw() { return this->m_filename; }

std::ostream &operator<<(std::ostream &os, const Exception &err) {
  if (!err.functionname().empty()) {
    os << "in function " << err.functionname() << " ";
  }
  os << err.what();
  if (!err.filename().empty()) {
    os << " in file " << err.filename();
  }
  return os;
}

} // namespace Mantid::Nexus
