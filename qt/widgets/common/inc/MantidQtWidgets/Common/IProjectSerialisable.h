// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include <stdexcept>
#include <string>
#include <vector>

class ApplicationWindow;

namespace MantidQt {
namespace API {

/**
Defines an interface to a MantidPlot class that can be saved into or loaded from
a project.

@author Harry Jeffery, ISIS, RAL
@date 31/07/2014
*/
class IProjectSerialisable {
public:
  /// Virtual destructor (required by linker on some versions of OS X/Intel
  /// compiler)
  virtual ~IProjectSerialisable() {}

  static IProjectSerialisable *loadFromProject(const std::string &lines, ApplicationWindow *app,
                                               const int fileVersion) {
    UNUSED_ARG(lines);
    UNUSED_ARG(app);
    UNUSED_ARG(fileVersion);
    throw std::runtime_error("Not implemented");
  }

  /// Serialises to a string that can be saved to a project file.
  virtual std::string saveToProject(ApplicationWindow *app) = 0;
  /// Returns a list of workspace names that are used by this window
  virtual std::vector<std::string> getWorkspaceNames() = 0;
  /// Returns the user friendly name of the window
  virtual std::string getWindowName() = 0;
  /// Returns the type of the window
  virtual std::string getWindowType() = 0;
};

} // namespace API
} // namespace MantidQt
