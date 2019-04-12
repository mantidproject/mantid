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
  virtual void notifyADSChanged(const QSet<QString> & /*unused*/) {}

  /// Return global options for pre-processing
  virtual ColumnOptionsQMap getPreprocessingOptions() const {
    return ColumnOptionsQMap();
  }
  /// Return global options for reduction
  virtual OptionsQMap getProcessingOptions() const { return OptionsQMap(); }
  /// Return global options for post-processing as a string
  virtual QString getPostprocessingOptionsAsString() const { return QString(); }
  /// Return time-slicing values
  virtual QString getTimeSlicingValues() const { return QString(); }
  /// Return time-slicing type
  virtual QString getTimeSlicingType() const { return QString(); }
  /// Return transmission runs for a particular angle
  virtual OptionsQMap getOptionsForAngle(const double) const {
    return OptionsQMap();
  }
  /// Return true if there are per-angle transmission runs set
  virtual bool hasPerAngleOptions() const { return false; }

  /// Return true if autoreduction is in progress
  virtual bool isAutoreducing() const { return false; }

  /// Handle data reduction paused/resumed
  virtual void pause() {}
  virtual void resume() const {}

  /// Handle data reduction paused/resumed confirmation
  virtual void confirmReductionCompleted() {}
  virtual void confirmReductionPaused(){};
  virtual void confirmReductionResumed(){};
  virtual void
  completedGroupReductionSuccessfully(GroupData const & /*unused*/,
                                      std::string const & /*unused*/){};
  virtual void
  completedRowReductionSuccessfully(GroupData const & /*unused*/,
                                    std::string const & /*unused*/){};
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /* MANTIDQTMANTIDWIDGETS_DATAPROCESSORMAINPRESENTER_H */
