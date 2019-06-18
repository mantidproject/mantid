// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_REFLAUTOREDUCTION_H
#define MANTID_ISISREFLECTOMETRY_REFLAUTOREDUCTION_H

#include "Common/DllConfig.h"
#include "IAutoreduction.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class Autoreduction

    Class to hold information about an autoreduction process
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL Autoreduction : public IAutoreduction {
public:
  Autoreduction();

  bool running() const override;
  bool searchStringChanged(const std::string &newSearchString) const override;
  bool searchResultsExist() const override;
  void setSearchResultsExist() override;

  void setupNewAutoreduction(const std::string &searchString) override;
  bool pause() override;
  void stop() override;

private:
  bool m_running;
  std::string m_searchString;
  bool m_searchResultsExist;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif /* MANTID_ISISREFLECTOMETRY_REFLAUTOREDUCTION_H */
