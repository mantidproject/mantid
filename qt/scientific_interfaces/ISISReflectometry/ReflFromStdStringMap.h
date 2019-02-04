// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_REFLFROMSTDSTRINGMAP_H
#define MANTID_CUSTOMINTERFACES_REFLFROMSTDSTRINGMAP_H
#include "DllConfig.h"
#include <QString>
#include <map>
#include <vector>
/**
This file contains some functions used to convert map data structures using
std::string
to those using QString.
*/
namespace MantidQt {
namespace CustomInterfaces {
std::map<QString, QString> MANTIDQT_ISISREFLECTOMETRY_DLL
fromStdStringMap(std::map<std::string, std::string> const &inMap);

std::vector<std::map<QString, QString>>
    MANTIDQT_ISISREFLECTOMETRY_DLL fromStdStringVectorMap(
        std::vector<std::map<std::string, std::string>> const &inVectorMap);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /*MANTID_CUSTOMINTERFACES_REFLFROMSTDSTRINGMAP_H*/
