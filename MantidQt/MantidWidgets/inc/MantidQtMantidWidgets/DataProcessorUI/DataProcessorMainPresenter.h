#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORMAINPRESENTER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORMAINPRESENTER_H

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorMainPresenter

DataProcessorMainPresenter is an interface that defines the functions that
need to be implemented to communicate (send/receive information) with a
DataProcessor presenter. Any interface that uses a DataProcessor widget should
have a concrete presenter inheriting from this interface. As an example,
ReflRunsTabPresenter (the presenter of the 'Runs' tab in the ISIS Reflectometry
(Polref) interface implements this interface to receive the list of actions,
including the list of available workspaces in the ADS, and populate the menus
'Reflectometry' and 'Edit'.

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DataProcessorMainPresenter {
public:
  virtual ~DataProcessorMainPresenter(){};

  enum Flag { ADSChangedFlag };

  /// Notify this receiver that something changed in the ADS
  virtual void notify(DataProcessorMainPresenter::Flag flag) = 0;

  /// Dialog/Prompt methods
  virtual std::string askUserString(const std::string &prompt,
                                    const std::string &title,
                                    const std::string &defaultValue) = 0;
  virtual bool askUserYesNo(std::string prompt, std::string title) = 0;
  virtual void giveUserWarning(std::string prompt, std::string title) = 0;
  virtual void giveUserCritical(std::string prompt, std::string title) = 0;
  virtual std::string runPythonAlgorithm(const std::string &algorithm) = 0;

  /// Return values to perform pre-processing on
  virtual std::map<std::string, std::string> getPreprocessingValues() const = 0;
  /// Return property names associated with pre-processing values
  virtual std::map<std::string, std::set<std::string>>
  getPreprocessingProperties() const = 0;
  /// Return global options for pre-processing
  virtual std::map<std::string, std::string>
  getPreprocessingOptions() const = 0;
  /// Return global options for reduction
  virtual std::string getProcessingOptions() const = 0;
  /// Return global options for post-processing
  virtual std::string getPostprocessingOptions() const = 0;
  /// Return time-slicing values
  virtual std::string getTimeSlicingValues() const = 0;
  /// Return time-slicing type
  virtual std::string getTimeSlicingType() const = 0;
};
}
}
#endif /* MANTIDQTMANTIDWIDGETS_DATAPROCESSORMAINPRESENTER_H */