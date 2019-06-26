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

int SpectrumRowIndexId = qRegisterMetaType<SpectrumRowIndex>();
int WorkspaceIndexId = qRegisterMetaType<WorkspaceIndex>();
int GroupIndexId = qRegisterMetaType<GroupIndex>();

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
