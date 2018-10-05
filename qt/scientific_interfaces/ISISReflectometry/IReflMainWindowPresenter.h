// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWPRESENTER_H
#define MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWPRESENTER_H

#include "IReflMainWindowView.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class IReflMainWindowPresenter

IReflMainWindowPresenter is the interface defining the functions that the main
window presenter needs to implement. This interface is used by tab presenters to
request information from other tabs.
*/
class IReflMainWindowPresenter : public ReflMainWindowSubscriber {
public:
  virtual std::string runPythonAlgorithm(const std::string &pythonCode) = 0;
  //  virtual void setInstrumentName(const std::string &instName) const = 0;
  virtual bool isProcessing() const = 0;
  virtual ~IReflMainWindowPresenter() = default;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IREFLMAINWINDOWPRESENTER_H */
