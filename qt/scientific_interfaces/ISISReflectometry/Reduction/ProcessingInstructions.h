// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_PROCESSINGINSTRUCTIONS_H_
#define MANTID_CUSTOMINTERFACES_PROCESSINGINSTRUCTIONS_H_

#include "Common/DllConfig.h"
#include <boost/optional.hpp>
namespace MantidQt {
namespace CustomInterfaces {

/** For now processing instructions are just a string but we expect them to
    become more complicated so this can be changed to a class in the future
*/
using ProcessingInstructions = std::string;
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_PROCESSINGINSTRUCTIONS_H_
