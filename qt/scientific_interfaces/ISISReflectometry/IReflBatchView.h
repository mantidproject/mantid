// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLBATCHVIEW_H
#define MANTID_ISISREFLECTOMETRY_IREFLBATCHVIEW_H

#include "GUI/Event/IEventView.h"
#include "GUI/Experiment/IExperimentView.h"
#include "GUI/Instrument/IInstrumentView.h"
#include "GUI/Runs/IRunsView.h"
#include "GUI/Save/ISaveView.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflBatchView

IReflBatchView is the interface defining the functions that the main
window view needs to implement. It is empty and not necessary at the moment, but
can be used in the future if widgets common to all tabs are added, for instance,
the help button.
*/
class IReflBatchView {
public:
  virtual IRunsView *runs() const = 0;
  virtual IEventView *eventHandling() const = 0;
  virtual ISaveView *save() const = 0;
  virtual IExperimentView *experiment() const = 0;
  virtual IInstrumentView *instrument() const = 0;
  virtual ~IReflBatchView() = default;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLBATCHVIEW_H */
