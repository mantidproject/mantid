// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "RangeInQ.h"
#include "TransmissionRunPair.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>
#include <map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<std::vector<std::string>> parseRunNumbers(std::string const &runNumbers);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<std::string> parseRunNumber(std::string const &runNumberString);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<std::string>
parseRunNumberOrWhitespace(std::string const &runNumberString);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double> parseTheta(std::string const &theta);

MANTIDQT_ISISREFLECTOMETRY_DLL std::optional<boost::regex> parseTitleMatcher(std::string const &titleMatcher);

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::variant<TransmissionRunPair, std::vector<int>> parseTransmissionRuns(std::string const &firstTransmissionRun,
                                                                            std::string const &secondTransmissionRun);

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::variant<RangeInQ, std::vector<int>> parseQRange(std::string const &min, std::string const &max,
                                                       std::string const &step);

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::optional<boost::optional<double>> parseScaleFactor(std::string const &scaleFactor);

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::optional<std::map<std::string, std::string>> parseOptions(std::string const &options);

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::optional<boost::optional<std::string>> parseProcessingInstructions(std::string const &instructions);

MANTIDQT_ISISREFLECTOMETRY_DLL
boost::optional<std::vector<std::string>> parseTitleAndThetaFromRunTitle(std::string const &runTitle);

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
