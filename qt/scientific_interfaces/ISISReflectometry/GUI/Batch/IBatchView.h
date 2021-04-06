// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Event/IEventView.h"
#include "GUI/Experiment/IExperimentView.h"
#include "GUI/Instrument/IInstrumentView.h"
#include "GUI/Runs/IRunsView.h"
#include "GUI/Save/ISaveView.h"
#include <string>

namespace MantidQt {

namespace API {
class BatchAlgorithmRunner;
class IConfiguredAlgorithm;
using IConfiguredAlgorithm_sptr = std::shared_ptr<IConfiguredAlgorithm>;
} // namespace API

namespace CustomInterfaces {
namespace ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL BatchViewSubscriber {
public:
  virtual void notifyBatchComplete(bool error) = 0;
  virtual void notifyBatchCancelled() = 0;
  virtual void notifyAlgorithmStarted(API::IConfiguredAlgorithm_sptr algorithm) = 0;
  virtual void notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr algorithm) = 0;
  virtual void notifyAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm, std::string const &message) = 0;
};

/** @class IBatchView

IBatchView is the base view class for the Reflectometry "Batch"
tab. It contains no QT specific functionality as that should be handled by a
subclass.
*/
class IBatchView {
public:
  virtual ~IBatchView() = default;
  virtual void subscribe(BatchViewSubscriber *notifyee) = 0;
  virtual IRunsView *runs() const = 0;
  virtual IEventView *eventHandling() const = 0;
  virtual ISaveView *save() const = 0;
  virtual IExperimentView *experiment() const = 0;
  virtual IInstrumentView *instrument() const = 0;
  virtual void clearAlgorithmQueue() = 0;
  virtual void setAlgorithmQueue(std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> algorithms) = 0;
  virtual void executeAlgorithmQueue() = 0;
  virtual void cancelAlgorithmQueue() = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt