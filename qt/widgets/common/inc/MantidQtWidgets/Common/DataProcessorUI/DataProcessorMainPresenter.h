#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORMAINPRESENTER_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORMAINPRESENTER_H

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

#include <QSet>
#include <QString>
#include <map>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

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
  virtual ~DataProcessorMainPresenter() {}

  /// Notify this receiver with the list of table workspaces in the ADS that can
  /// be loaded into the interface
  virtual void notifyADSChanged(const QSet<QString> &, int) {}

  /// Return global options for pre-processing
  virtual ColumnOptionsQMap getPreprocessingOptions(int) const {
    return ColumnOptionsQMap();
  }
  /// Return global options for reduction
  virtual OptionsQMap getProcessingOptions(int) const { return OptionsQMap(); }
  /// Return global options for post-processing as a string
  virtual QString getPostprocessingOptionsAsString(int) const {
    return QString();
  }
  /// Return time-slicing values
  virtual QString getTimeSlicingValues(int) const { return QString(); }
  /// Return time-slicing type
  virtual QString getTimeSlicingType(int) const { return QString(); }
  /// Return transmission runs for a particular angle
  virtual OptionsQMap getOptionsForAngle(const double, int) const {
    return OptionsQMap();
  }
  /// Return true if there are per-angle transmission runs set
  virtual bool hasPerAngleOptions(int) const { return false; }

  /// Return true if autoreduction is in progress for any group
  virtual bool isAutoreducing() const { return false; }
  /// Return true if autoreduction is in progress for a specific group
  virtual bool isAutoreducing(int) const { return false; }

  /// Handle data reduction paused/resumed
  virtual void pause(int) {}
  virtual void resume(int) const {}

  /// Handle data reduction paused/resumed confirmation
  virtual void confirmReductionCompleted(int) {}
  virtual void confirmReductionPaused(int){};
  virtual void confirmReductionResumed(int){};
  virtual void completedGroupReductionSuccessfully(GroupData const &,
                                                   std::string const &){};
  virtual void completedRowReductionSuccessfully(GroupData const &,
                                                 std::string const &){};
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /* MANTIDQTMANTIDWIDGETS_DATAPROCESSORMAINPRESENTER_H */
