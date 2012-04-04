#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidKernel/System.h"
#include <stdexcept>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DataProcessorAlgorithm::DataProcessorAlgorithm()
  {
    m_loadAlg = "Load";
    m_accumulateAlg = "Plus";
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DataProcessorAlgorithm::~DataProcessorAlgorithm()
  {
  }

  //----------------------------------------------------------------------------------------------
  void DataProcessorAlgorithm::setLoadAlg(const std::string & alg)
  {
    if (alg.empty())
      throw std::invalid_argument("Cannot set load algorithm to empty string");
    m_loadAlg = alg;
  }

  void DataProcessorAlgorithm::setAccumAlg(const std::string &alg)
  {
    if (alg.empty())
      throw std::invalid_argument("Cannot set accumulate algorithm to empty string");
    m_accumulateAlg = alg;
  }

  ITableWorkspace_sptr DataProcessorAlgorithm::determineChunk()
  {
    throw std::runtime_error("DataProcessorAlgorithm::determineChunk is not implemented");
  }

  void DataProcessorAlgorithm::loadChunk()
  {
    throw std::runtime_error("DataProcessorAlgorithm::loadChunk is not implemented");
  }

  void DataProcessorAlgorithm::load()
  {
    throw std::runtime_error("DataProcessorAlgorithm::load is not implemented");
  }

  std::vector<std::string> DataProcessorAlgorithm::splitInput(const std::string & input)
  {
    UNUSED_ARG(input);
    throw std::runtime_error("DataProcessorAlgorithm::splitInput is not implemented");
  }

  void DataProcessorAlgorithm::forwardProperties()
  {
    throw std::runtime_error("DataProcessorAlgorithm::forwardProperties is not implemented");
  }
} // namespace Mantid
} // namespace API
