// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_IAUTOREDUCTION_H
#define MANTID_ISISREFLECTOMETRY_IAUTOREDUCTION_H

#include "Common/DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class Autoreduction

Class to hold information about an autoreduction process
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL IAutoreduction {
public:
  virtual ~IAutoreduction() = default;

  virtual bool running() const = 0;
  virtual bool
  searchStringChanged(const std::string &newSearchString) const = 0;
  virtual bool searchResultsExist() const = 0;
  virtual void setSearchResultsExist() = 0;

  virtual void setupNewAutoreduction(const std::string &searchString) = 0;
  virtual bool pause() = 0;
  virtual void stop() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_IAUTOREDUCTION_H */
