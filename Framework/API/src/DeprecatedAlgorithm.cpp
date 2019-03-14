// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidTypes/Core/DateAndTimeHelpers.h"
#include <sstream>

namespace Mantid {
namespace API {
namespace {
/// Static logger
Kernel::Logger g_log("DeprecatedAlgorithm");
} // namespace

/// Does nothing other than make the compiler happy.
DeprecatedAlgorithm::DeprecatedAlgorithm()
    : m_replacementAlgorithm(), m_replacementVersion(-1), m_deprecatedDate() {}

/// Does nothing other than make the compiler happy.
DeprecatedAlgorithm::~DeprecatedAlgorithm() = default;

/// The algorithm to use instead of this one.
/// @param replacement Name of the algorithm that replaces the deprecated one
/// @param version An optional version number for the replacement (default=-1)
void DeprecatedAlgorithm::useAlgorithm(const std::string &replacement,
                                       const int version) {
  // Replacement is not checked here as we cannot guarantee all algorithms have
  // been
  // registered when we are called. It is checked when the message is about to
  // be printed
  if (!replacement.empty())
    this->m_replacementAlgorithm = replacement;
  else
    this->m_replacementAlgorithm = "";

  m_replacementVersion = version;
}

/// The date the algorithm was deprecated on
void DeprecatedAlgorithm::deprecatedDate(const std::string &date) {
  this->m_deprecatedDate = "";
  if (date.empty()) {
    // TODO warn people that it wasn't set
    return;
  }
  if (!Types::Core::DateAndTimeHelpers::stringIsISO8601(date)) {
    // TODO warn people that it wasn't set
    return;
  }
  this->m_deprecatedDate = date;
}

/// This merely prints the deprecation error for people to see.
std::string DeprecatedAlgorithm::deprecationMsg(const IAlgorithm *algo) {
  std::stringstream msg;
  if (algo != nullptr)
    msg << algo->name() << " is ";

  msg << "deprecated";

  if (!this->m_deprecatedDate.empty())
    msg << " (on " << this->m_deprecatedDate << ")";

  if (this->m_replacementAlgorithm.empty()) {
    msg << " and has no replacement.";
  } else {
    // sanity check
    if (!AlgorithmFactory::Instance().exists(this->m_replacementAlgorithm,
                                             this->m_replacementVersion)) {
      std::ostringstream invalidReplacementMsg;
      invalidReplacementMsg << "Invalid replacement algorithm '" +
                                   this->m_replacementAlgorithm + "'";
      if (this->m_replacementVersion > 0)
        invalidReplacementMsg << " version " << this->m_replacementVersion
                              << "\n";
      invalidReplacementMsg << "Replacement algorithm not registered.";
      g_log.warning(invalidReplacementMsg.str());
    }

    msg << ". Use " << this->m_replacementAlgorithm;
    if (this->m_replacementVersion > 0)
      msg << " version " << this->m_replacementVersion;
    msg << " instead.";
  }

  return msg.str();
}
} // namespace API
} // namespace Mantid
