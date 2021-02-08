// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/IndexTypes.h"
#include <QMetaType>

namespace MantidQt {
namespace MantidWidgets {

// The return variables here are needed in order for the code to compile.
int SpectrumRoqIndexId = qRegisterMetaType<FitDomainIndex>();
int WorkspaceIndexId = qRegisterMetaType<WorkspaceIndex>();
int GrouppIndexId = qRegisterMetaType<WorkspaceGroupIndex>();

} // namespace MantidWidgets
} // namespace MantidQt
