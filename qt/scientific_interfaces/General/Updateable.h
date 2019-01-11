// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_UPDATABLE_H
#define MANTIDQTCUSTOMINTERFACES_UPDATABLE_H

#include "MantidKernel/System.h"

namespace MantidQt {
namespace CustomInterfaces {
/** Abstraction of an updateable item, i.e. a MVP presenter or Qt MVC Model.

 @author Owen Arnold, RAL ISIS
 @date 06/Oct/2011
*/
class DLLExport Updateable {
public:
  virtual void update() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif