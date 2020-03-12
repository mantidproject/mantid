
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadMuonStrategy.h"

namespace Mantid {
namespace DataHandling {

// Constructor
LoadMuonStrategy::LoadMuonStrategy(Kernel::Logger &g_log,
                                   const std::string &filename)
    : m_logger(g_log), m_filename(filename) {}

} // namespace DataHandling
} // namespace Mantid
