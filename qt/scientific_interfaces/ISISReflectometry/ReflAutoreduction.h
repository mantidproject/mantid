#ifndef MANTID_ISISREFLECTOMETRY_REFLAUTOREDUCTION_H
#define MANTID_ISISREFLECTOMETRY_REFLAUTOREDUCTION_H

#include "DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class Autoreduction

Class to hold information about an autoreduction process
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflAutoreduction {
public:
  ReflAutoreduction();

  bool running() const;
  int group() const;
  bool searchStringChanged(const std::string &newSearchString) const;
  bool searchResultsExist() const;
  void setSearchResultsExist();

  bool setupNewAutoreduction(const int group, const std::string &searchString);
  bool pause(int group);
  void stop();

private:
  bool m_running;
  int m_group;
  std::string m_searchString;
  bool m_searchResultsExist;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_REFLAUTOREDUCTION_H */
