// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INODE_H_
#define INODE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ISaveable.h"

namespace Mantid {
namespace Kernel {
/** Helper class providing interface to ISAveable
 */
class DLLExport INode {
public:
  virtual ~INode(){};
};
} // namespace Kernel
} // namespace Mantid

#endif