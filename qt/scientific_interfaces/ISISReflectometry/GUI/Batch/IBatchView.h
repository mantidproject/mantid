// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Common/IJobRunner.h"
#include "GUI/Event/IEventView.h"
#include "GUI/Experiment/IExperimentView.h"
#include "GUI/Instrument/IInstrumentView.h"
#include "GUI/Preview/IPreviewView.h"
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

/** @class IBatchView

IBatchView is the base view class for the Reflectometry "Batch"
tab. It contains no QT specific functionality as that should be handled by a
subclass.
*/
class IBatchView : public IJobRunner {
public:
  virtual ~IBatchView() = default;
  virtual IRunsView *runs() const = 0;
  virtual IEventView *eventHandling() const = 0;
  virtual ISaveView *save() const = 0;
  virtual IExperimentView *experiment() const = 0;
  virtual IInstrumentView *instrument() const = 0;
  virtual IPreviewView *preview() const = 0;
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
