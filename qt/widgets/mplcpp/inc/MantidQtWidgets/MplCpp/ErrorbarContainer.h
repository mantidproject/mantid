// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_ERRORBARCONTAINER_H
#define MPLCPP_ERRORBARCONTAINER_H

#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Python/Object.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

class MANTID_MPLCPP_DLL ErrorbarContainer : public Python::InstanceHolder {
public:
  ErrorbarContainer(Python::Object pyInstance);
};

}}}

#endif // MPLCPP_ERRORBARCONTAINER_H
