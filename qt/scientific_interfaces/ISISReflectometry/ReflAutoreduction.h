#ifndef MANTID_ISISREFLECTOMETRY_REFLAUTOREDUCTION_H
#define MANTID_ISISREFLECTOMETRY_REFLAUTOREDUCTION_H

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** @class Autoreduction

Class to hold information about an autoreduction process
*/
class ReflAutoreduction {
public:
  /// Constructor
  ReflAutoreduction();

  /// Check whether autoreduction is currently running
  bool running() const;
  /// Get the group that autoreduction is running for
  int group() const;
  /// Return true if the given search string is different from when
  /// autoreduction was started
  bool searchStringChanged(const std::string &newSearchString) const;
  /// Check whether search results have been created yet
  bool searchResultsExist() const;
  /// Set the flag to indicate search results list has been created
  void setSearchResultsExist();

  /// Start an autoreduction on the given group
  bool start(const int group, const std::string &searchString);
  /// Stop an autoreduction on the given group
  bool stop(int group);
  /// Stop an autoreduction on any group
  bool stop();

private:
  /// Flag indicating whether autoreduction is currently running
  bool m_running;
  /// The group autoreduction is running on
  int m_group;
  /// The string that was used to start the autoreduction
  std::string m_searchString;
  /// Flag indicating that the search results list has been created
  bool m_searchResultsExist;
};
}
}
#endif /* MANTID_ISISREFLECTOMETRY_REFLAUTOREDUCTION_H */
