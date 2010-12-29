#ifndef EVENTWORKSPACEHELPERS_H_
#define EVENTWORKSPACEHELPERS_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace DataObjects
{

/*
 * EventWorkspaceHelpers.h
 *
 * A collection of functions that help for EventWorkspaces.
 *
 *  Created on: Dec 15, 2010
 *      Author: Janik Zikovsky
 */
struct DLLExport EventWorkspaceHelpers
{
  // Converts an EventWorkspace to an equivalent Workspace2D.
  static API::MatrixWorkspace_sptr convertEventTo2D(API::MatrixWorkspace_sptr inputMatrixW);
};



}//namespace Mantid
}//namespace DataObjects

#endif /* EVENTWORKSPACEHELPERS_H_ */
