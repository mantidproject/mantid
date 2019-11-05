// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "IndexTypes.h"
#include <QMetaType>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

// The return variables here are needed in order for the code to compile.
int SpectrumRoqIndexId = qRegisterMetaType<TableRowIndex>();
int WorkspaceIndexId = qRegisterMetaType<WorkspaceIndex>();
int GrouppIndexId = qRegisterMetaType<WorkspaceGroupIndex>();

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt