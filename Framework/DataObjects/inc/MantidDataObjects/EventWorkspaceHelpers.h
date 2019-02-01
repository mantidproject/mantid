// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef EVENTWORKSPACEHELPERS_H_
#define EVENTWORKSPACEHELPERS_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace DataObjects {

/** A collection of functions that help for EventWorkspaces.
 *
 *  Created on: Dec 15, 2010
 *      Author: Janik Zikovsky
 */
struct DLLExport EventWorkspaceHelpers {
  /// Converts an EventWorkspace to an equivalent Workspace2D.
  static API::MatrixWorkspace_sptr
  convertEventTo2D(API::MatrixWorkspace_sptr inputMatrixW);
};

} // namespace DataObjects
} // namespace Mantid

#endif /* EVENTWORKSPACEHELPERS_H_ */
