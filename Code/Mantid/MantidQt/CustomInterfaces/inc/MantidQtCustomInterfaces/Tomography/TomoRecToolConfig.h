#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECTOOLCONFIG_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECTOOLCONFIG_H_

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/**
Configuration of a third party tomographic reconstruction tool. This
is under development, and as it is not necessarily related to custom
interfaces this class and some derived ones might be moved out of
here.

Copyright &copy; 2014,205 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/**
 * General tomographic reconstruction tool configuration. This class
 * represents the configuration (or run options/settings) of a third
 * party or external tool/program/module/etc. that Mantid would run
 * either locally or on a remote computer. The purpose here is to
 * produce the command line or similar that will run the third party
 * tool for certain user requirements/settings/options/preferences.
 *
 * TODO: a related class to represent third party tools should be
 * added that would take care of platform specificities, tool
 * availability, execute permissions, etc.
 */
class TomoRecToolConfig {
public:
  /**
   * Construct a config object, given a 'runnable', which can be an
   * application, executable, script, etc. - the access point to a
   * third party tool. If this class is developed to be smart enough,
   * and with the help of the additional '3rd party tool' class, it
   * should be able to translate application names into binary paths
   * and similar internally.
   *
   * @param runnable name of the runnable object (application, executable,
   * etc.). This can be a full path, an application name, etc. At
   * present it is used in its simplest form: platform and machine
   * dependent full path to an execuatable or script.
   */
  TomoRecToolConfig(const std::string &runnable="") : m_runnable(runnable) {}

  virtual ~TomoRecToolConfig() {}

  /**
   * validate that it is possible to produce a sensible command line
   * from this config object.
   *
   * @return the tool and its config are consistent and it looks
   * like it should be possible to run it.
   */
  virtual bool valid() const { return true; }

  /**
   * Produce a command line to run this tool with this configuration.
   *
   * @return command line ready to run on a certain platform
   */
  virtual std::string toCommand() const {
    return makeExecutable() + " " + makeCmdLineOptions();
  }

  /**
   * Produces a string with the command line options derived from the
   * different options set.
   *
   * @return command line options string
   */
  virtual std::string makeCmdLineOptions() const = 0;

  /**
   * Produces an string that describes the executable, ready to run
   * as a binary, python or other type of scripts, etc. Normally you
   * append command line options to this.
   */
  virtual std::string makeExecutable() const = 0;

protected:
  std::string m_runnable;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECTOOLCONFIG_H_
