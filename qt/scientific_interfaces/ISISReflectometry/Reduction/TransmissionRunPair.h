// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
/** @class TransmissionRunPair

    The TransmissionRunPair model holds information about the two possible
    input transmission workspaces for a reduction. Each workspace may be
    constructed from multiple runs, which will be summed prior to reduction.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL TransmissionRunPair {
public:
  TransmissionRunPair();
  TransmissionRunPair(std::string firstTransmissionRun, std::string secondTransmissionRun);
  TransmissionRunPair(std::vector<std::string> firstTransmissionRunNumbers,
                      std::vector<std::string> secondTransmissionRunNumbers);

  std::vector<std::string> const &firstTransmissionRunNumbers() const;
  std::vector<std::string> const &secondTransmissionRunNumbers() const;
  std::string firstRunList() const;
  std::string secondRunList() const;

private:
  std::vector<std::string> m_firstTransmissionRunNumbers;
  std::vector<std::string> m_secondTransmissionRunNumbers;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(TransmissionRunPair const &lhs, TransmissionRunPair const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(TransmissionRunPair const &lhs, TransmissionRunPair const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
