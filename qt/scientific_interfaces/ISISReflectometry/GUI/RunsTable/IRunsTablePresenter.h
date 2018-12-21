// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLRUNSTABLEPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLRUNSTABLEPRESENTER_H

#include "IReflBatchPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsMap.h"

namespace MantidQt {
namespace CustomInterfaces {

class IRunsPresenter;
class ReductionJobs;

/** @class IRunsTablePresenter

IRunsTablePresenter is an interface which defines the functions any
reflectometry interface presenter needs to support.
*/
class IRunsTablePresenter {
public:
  virtual ~IRunsTablePresenter() = default;
  virtual void acceptMainPresenter(IRunsPresenter *mainPresenter) = 0;
  virtual ReductionJobs const &reductionJobs() const = 0;

  virtual void mergeAdditionalJobs(ReductionJobs const &jobs) = 0;
  virtual void setInstrumentName(std::string const &instrumentName) = 0;

  virtual void reductionPaused() = 0;
  virtual void reductionResumed() = 0;
  virtual void autoreductionPaused() = 0;
  virtual void autoreductionResumed() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLRUNSTABLEPRESENTER_H */
