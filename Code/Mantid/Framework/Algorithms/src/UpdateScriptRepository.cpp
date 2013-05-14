/*WIKI*
It updates the [[ScriptRepository]]. It checkout the information of the central repository
and download all the files marked for AutoUpdate.
*WIKI*/

#include "MantidAlgorithms/UpdateScriptRepository.h"
#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/ScriptRepository.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(UpdateScriptRepository)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  UpdateScriptRepository::UpdateScriptRepository()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  UpdateScriptRepository::~UpdateScriptRepository()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string UpdateScriptRepository::name() const { return "UpdateScriptRepository";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int UpdateScriptRepository::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string UpdateScriptRepository::category() const { return "Utility";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void UpdateScriptRepository::initDocs()
  {
    this->setWikiSummary("Update the local instance of [[ScriptRepository]].");
    this->setOptionalMessage("Update the local instance of [[ScriptRepository]].");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void UpdateScriptRepository::init()
  {

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void UpdateScriptRepository::exec()
  {
    // TODO Auto-generated execute stub
    using Mantid::API::ScriptRepositoryFactory;
    using Mantid::API::ScriptRepository_sptr;
    using Mantid::API::ScriptRepository;
    auto repo_ptr = ScriptRepositoryFactory::Instance().create("ScriptRepositoryImpl");
    
    try{
      std::vector<std::string> f_list = repo_ptr->check4Update();
      if (f_list.size() > 0){
        declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>("UpdatedFileList","DownloadedFiles",Kernel::Direction::Output), "The list containing all the files whose new version was downloaded.");
        auto m_list = API::WorkspaceFactory::Instance().createTable("TableWorkspace");
        m_list->addColumn("str","file path");
        std::stringstream info; 
        info << "ScriptRepository Update Info:\n" 
             << " The following files were updated:\n";
        for(unsigned short i = 0; i< f_list.size();i++){
          API::TableRow t = m_list->appendRow();
          t << f_list[i];
          info << "  * " << f_list[i] << "\n"; 
        }
        g_log.notice() << info.str() << std::endl; 
        setProperty("UpdatedFileList", m_list);      
      }
    }catch(API::ScriptRepoException & ex){
      g_log.error() << ex.what() << std::endl; 
    }
  }



} // namespace Algorithms
} // namespace Mantid
