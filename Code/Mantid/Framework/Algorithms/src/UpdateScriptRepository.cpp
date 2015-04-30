#include "MantidAlgorithms/UpdateScriptRepository.h"
#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidAPI/ScriptRepository.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(UpdateScriptRepository)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
UpdateScriptRepository::UpdateScriptRepository() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
UpdateScriptRepository::~UpdateScriptRepository() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string UpdateScriptRepository::name() const {
  return "UpdateScriptRepository";
}

/// Algorithm's version for identification. @see Algorithm::version
int UpdateScriptRepository::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string UpdateScriptRepository::category() const { return "Utility"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void UpdateScriptRepository::init() {}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void UpdateScriptRepository::exec() {
  // TODO Auto-generated execute stub
  using Mantid::API::ScriptRepositoryFactory;
  using Mantid::API::ScriptRepository_sptr;
  using Mantid::API::ScriptRepository;
  auto repo_ptr =
      ScriptRepositoryFactory::Instance().create("ScriptRepositoryImpl");

  if (!repo_ptr->isValid())
    return; // it means that the ScriptRepository was not installed.

  std::vector<std::string> f_list = repo_ptr->check4Update();
  if (f_list.size() > 0) {
    std::stringstream info;
    info << "Information about ScriptRepository:\n"
         << " A more recent version of the following files was installed:\n";
    for (unsigned short i = 0; i < f_list.size(); i++) {
      info << "  * " << f_list[i] << "\n";
    }
    info << "Please check these files before using them. "
         << "Note: These files were configured for AutoUpdate.";
    g_log.warning() << info.str() << std::endl;
  }
}

} // namespace Algorithms
} // namespace Mantid
