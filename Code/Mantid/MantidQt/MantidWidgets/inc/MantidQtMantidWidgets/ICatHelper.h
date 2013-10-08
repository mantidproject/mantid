#ifndef MANTIDQTWIDGETS_ICATHELPER_H_
#define MANTIDQTWIDGETS_ICATHELPER_H_

#include "MantidAPI/AlgorithmManager.h"

namespace MantidQt
{
  namespace MantidWidgets
  {
    class ICatHelper
    {

    public:
      /// Obtain the list of instruments that are available.
      std::vector<std::string> getInstrumentList();
      /// Obtain the list of instruments that are available.
      std::vector<std::string> getInvestigationTypeList();
      /// Run the search algorithm with the given user input.
      void executeSearch(std::map<std::string, std::string> userInputs);
      /// Search for all related dataFiles for the specified investigation.
      void executeGetDataFiles(int64_t investigationId);
      /// Retrieve the path(s) to the file that was downloaded (via HTTP) or is stored in the archive
      std::vector<std::string> getDataFilePaths(std::vector<std::pair<int64_t, std::string>>& userSelectedFiles);
    };
  } // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTWIDGETS_ICATHELPER_H_
