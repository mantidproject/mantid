// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
*/
class DataProcessorMainPresenter {
public:
  virtual ~DataProcessorMainPresenter() {}

  /// Notify this receiver with the list of table workspaces in the ADS that can
  /// be loaded into the interface
  virtual void notifyADSChanged(const QSet<QString> & /*unused*/, int /*unused*/) {}

  /// Return global options for pre-processing
  virtual ColumnOptionsQMap getPreprocessingOptions(int /*unused*/) const {
    return ColumnOptionsQMap();
  }
  /// Return global options for reduction
  virtual OptionsQMap getProcessingOptions(int /*unused*/) const { return OptionsQMap(); }
  /// Return global options for post-processing as a string
  virtual QString getPostprocessingOptionsAsString(int /*unused*/) const {
    return QString();
  }
  /// Return time-slicing values
  virtual QString getTimeSlicingValues(int /*unused*/) const { return QString(); }
  /// Return time-slicing type
  virtual QString getTimeSlicingType(int /*unused*/) const { return QString(); }
  /// Return transmission runs for a particular angle
  virtual OptionsQMap getOptionsForAngle(const double /*unused*/, int /*unused*/) const {
    return OptionsQMap();
  }
  /// Return true if there are per-angle transmission runs set
  virtual bool hasPerAngleOptions(int /*unused*/) const { return false; }

  /// Return true if autoreduction is in progress for any group
  virtual bool isAutoreducing() const { return false; }
  /// Return true if autoreduction is in progress for a specific group
  virtual bool isAutoreducing(int /*unused*/) const { return false; }

  /// Handle data reduction paused/resumed
  virtual void pause(int /*unused*/) {}
  virtual void resume(int /*unused*/) const {}

  /// Handle data reduction paused/resumed confirmation
  virtual void confirmReductionCompleted(int /*unused*/) {}
  virtual void confirmReductionPaused(int /*unused*/){};
  virtual void confirmReductionResumed(int /*unused*/){};
  virtual void completedGroupReductionSuccessfully(GroupData const & /*unused*/,
                                                   std::string const & /*unused*/){};
  virtual void completedRowReductionSuccessfully(GroupData const & /*unused*/,
                                                 std::string const & /*unused*/){};
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /* MANTIDQTMANTIDWIDGETS_DATAPROCESSORMAINPRESENTER_H */
