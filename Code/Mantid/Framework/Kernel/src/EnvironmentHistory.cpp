//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/EnvironmentHistory.h"
#include "MantidKernel/ConfigService.h"

// This file is created automatically by Mantid/Code/Mantid/build.bat(sh)
// and defines MANTID_VERSION
#include "MantidKernel/MantidVersion.h"

namespace Mantid
{
namespace Kernel
{
/// Default Constructor
/// @todo Populate the Framework Version & User fields with something meaningful
EnvironmentHistory::EnvironmentHistory() :
  m_version(MANTID_VERSION),
  m_osName(ConfigService::Instance().getOSName()),
  m_osVersion(ConfigService::Instance().getOSVersion()),
  m_userName()
{}

/// Destructor
EnvironmentHistory::~EnvironmentHistory()
{}

/**
  Standard Copy Constructor
  @param A :: AlgorithmParameter Item to copy
 */
EnvironmentHistory::EnvironmentHistory(const EnvironmentHistory& A) :
  m_version(A.m_version),m_osName(A.m_osName),m_osVersion(A.m_osVersion),m_userName(A.m_userName)
{}

/** Prints a text representation of itself
 *  @param os :: The ouput stream to write to
 *  @param indent :: an indentation value to make pretty printing of object and sub-objects
 */
void EnvironmentHistory::printSelf(std::ostream& os, const int indent) const
{
  os << std::string(indent,' ') << "Framework Version: " << m_version << std::endl;
  os << std::string(indent,' ') << "OS name: " << m_osName << std::endl;
  os << std::string(indent,' ') << "OS version: " << m_osVersion << std::endl;
  os << std::string(indent,' ') << "username: "<< m_userName << std::endl;
}

/** Prints a text representation
 *  @param os :: The ouput stream to write to
 *  @param EH :: The EnvironmentHistory to output
 *  @returns The ouput stream
 */
std::ostream& operator<<(std::ostream& os, const EnvironmentHistory& EH)
{
  EH.printSelf(os);
  return os;
}

} // namespace Kernel
} // namespace Mantid
