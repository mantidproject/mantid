//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ConfigService.h"

namespace Mantid
{
namespace API
{

/// Private constructor for singleton class
WorkspaceFactoryImpl::WorkspaceFactoryImpl() :
  Mantid::Kernel::DynamicFactory<Workspace>(), g_log(Kernel::Logger::get("WorkspaceFactory"))
{
  g_log.debug() << "WorkspaceFactory created." << std::endl;
}

/** Private destructor
 *  Prevents client from calling 'delete' on the pointer handed
 *  out by Instance
 */
WorkspaceFactoryImpl::~WorkspaceFactoryImpl()
{
  //	g_log.debug() << "WorkspaceFactory destroyed." << std::endl;
}

/** Create a new instance of the same type of workspace as that given as argument.
 *  If the optional size parameters are given, the workspace will be initialised using
 *  those; otherwise it will be initialised to the same size as the parent.
 *  This method should be used when you want to carry over the Workspace data members
 *  relating to the Instrument, Spectra-Detector Map, Sample & Axes to the new workspace.
 *  If the workspace is the same size as its parent, then the X data, axes and mask list are
 *  copied. If its a different size then they are not.
 *  @param  parent    A shared pointer to the parent workspace
 *  @param  NVectors  (Optional) The number of vectors/histograms/detectors in the workspace
 *  @param  XLength   (Optional) The number of X data points/bin boundaries in each vector (must all be the same)
 *  @param  YLength   (Optional) The number of data/error points in each vector (must all be the same)
 *  @return A shared pointer to the newly created instance
 *  @throw  std::out_of_range If invalid (0 or less) size arguments are given
 *  @throw  NotFoundException If the class is not registered in the factory
 */
MatrixWorkspace_sptr WorkspaceFactoryImpl::create(const MatrixWorkspace_const_sptr& parent,
                                            int NVectors, int XLength, int YLength) const
{

  // Flag to indicate whether this workspace is the same size as the parent
  bool differentSize = true;
  if ( YLength < 0 ) differentSize = false;

  // If the size parameters have not been specified, get them from the parent
  if ( !differentSize )
  {
    // Find out the size of the parent
    XLength = parent->dataX(0).size();
    YLength = parent->blocksize();
    NVectors = parent->size() / YLength;
  }

  MatrixWorkspace_sptr ws = create(parent->id(),NVectors,XLength,YLength);

  // Copy over certain parent data members
  ws->setInstrument(parent->getInstrument());
  ws->m_spectramap = parent->m_spectramap;
  ws->setSample(parent->getSample());
  ws->setYUnit(parent->m_YUnit);
  ws->isDistribution(parent->isDistribution());

  // Only copy the axes over if new sizes are not given
  if ( !differentSize )
  {
    // Copy X values over if same size as parent. 99% of the time, this will be what we want
    // If common X values, make sure they are shared...
    if ( WorkspaceHelpers::commonBoundaries(parent) )
    {
      Kernel::cow_ptr<MantidVec> XVals;
      XVals.access() = parent->readX(0);
      ws->setX(XVals);
    }
    // ... otherwise copy over each vector separately
    else
    {
      for (int j = 0; j < NVectors; ++j) ws->dataX(j) = parent->readX(j);
    }
    
    // Only copy mask map if same size for now. Later will need to check continued validity.
    ws->m_masks = parent->m_masks;
    
    for (unsigned int i = 0; i < parent->m_axes.size(); ++i)
    {
      // Need to delete the existing axis created in init above
      delete ws->m_axes[i];
      // Now set to a copy of the parent workspace's axis
      ws->m_axes[i] = parent->m_axes[i]->clone(ws.get());
    }
  }
  else
  {
    // Just copy the unit and title
    for (unsigned int i = 0; i < ws->m_axes.size(); ++i)
    {
      ws->getAxis(i)->unit() = parent->getAxis(i)->unit();
      ws->getAxis(i)->title() = parent->getAxis(i)->title();
    }
  }

  return ws;
}

/** Creates a new instance of the class with the given name, and allocates memory for the arrays
 *  where it creates and initialises either a Workspace1D, Workspace2D or a ManagedWorkspace2D
 *  according to the size requested and the value of the configuration parameter
 *  ManagedWorkspace.LowerMemoryLimit (default 40% of available physical memory) Workspace2D only.
 *  @param  className The name of the class you wish to create
 *  @param  NVectors  The number of vectors/histograms/detectors in the workspace
 *  @param  XLength   The number of X data points/bin boundaries in each vector (must all be the same)
 *  @param  YLength   The number of data/error points in each vector (must all be the same)
 *  @return A shared pointer to the newly created instance
 *  @throw  std::out_of_range If invalid (0 or less) size arguments are given
 *  @throw  NotFoundException If the class is not registered in the factory
 */
MatrixWorkspace_sptr WorkspaceFactoryImpl::create(const std::string& className, const int& NVectors,
                                            const int& XLength, const int& YLength) const
{
  MatrixWorkspace_sptr ws;

  // Creates a managed workspace if over the trigger size and a 2D workspace is being requested.
  // Otherwise calls the vanilla create method.
  bool is2D = className.find("2D") != std::string::npos;
  if ( MemoryManager::Instance().goForManagedWorkspace(NVectors,XLength,YLength) && is2D )
  {
      // check if there is enough memory for 100 data blocks
      int blockMemory;
      if ( ! Kernel::ConfigService::Instance().getValue("ManagedWorkspace.DataBlockSize", blockMemory)
          || blockMemory <= 0 )
      {
        // default to 1MB if property not found
        blockMemory = 1024*1024;
      }

      MemoryInfo mi = MemoryManager::Instance().getMemoryInfo();
      if ( blockMemory*100/1024 > mi.availMemory )
      {
          g_log.error("There is not enough memory to allocate the workspace");
          throw std::runtime_error("There is not enough memory to allocate the workspace");
      }

      ws = boost::dynamic_pointer_cast<MatrixWorkspace>(this->create("ManagedWorkspace2D"));
      g_log.information("Created a ManagedWorkspace2D");
  }
  else
  {
      // No need for a Managed Workspace
      if ( is2D && className.substr(0,7) == "Managed" )
          ws = boost::dynamic_pointer_cast<MatrixWorkspace>(this->create("Workspace2D"));
      else
          ws = boost::dynamic_pointer_cast<MatrixWorkspace>(this->create(className));
  }

  if (!ws)
  {
      g_log.error("Workspace was not created");
      throw std::runtime_error("Workspace was not created");
  }
  ws->initialize(NVectors,XLength,YLength);
  return ws;
}

/// Create a ITableWorkspace
ITableWorkspace_sptr WorkspaceFactoryImpl::createTable(const std::string& className) const
{
    ITableWorkspace_sptr ws;
    try
    {
        ws = boost::dynamic_pointer_cast<ITableWorkspace>(this->create(className));    
        if (!ws)
        {
            g_log.error("Class "+className+" cannot be cast to ITableWorkspace");
            throw std::runtime_error("Class "+className+" cannot be cast to ITableWorkspace");
        }
    }
    catch(Kernel::Exception::NotFoundError& e)
    {
        g_log.error(e.what());
        throw;
    }
    return ws;
}

} // namespace API
} // Namespace Mantid
