//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"


namespace Mantid
{
namespace API
{

using std::size_t;

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
    size_t NVectors, size_t XLength, size_t YLength) const
{

  // Flag to indicate whether this workspace is the same size as the parent
  bool differentSize = true;
  if ( YLength == size_t(-1) ) differentSize = false;

  // If the size parameters have not been specified, get them from the parent
  if ( !differentSize )
  {
    // Find out the size of the parent
    XLength = parent->dataX(0).size();
    YLength = parent->blocksize();
    NVectors = parent->getNumberHistograms();
  }

  // If the parent is an EventWorkspace, we want it to spawn a Workspace2D (or managed variant) as a child
  std::string id (parent->id());
  if ( id == "EventWorkspace" ) id = "Workspace2D";

  // Create an 'empty' workspace of the appropriate type and size
  MatrixWorkspace_sptr ws = create(id,NVectors,XLength,YLength);

  // Copy over certain parent data members
  initializeFromParent(parent,ws,differentSize);

  return ws;
}

/** Initialise a workspace from its parent
 * This sets values such as title, instrument, units, sample, spectramap.
 * This does NOT copy any data.
 *
 * @param parent :: the parent workspace
 * @param child :: the child workspace
 * @param differentSize :: A flag to indicate if the two workspace will be different sizes
 */
void WorkspaceFactoryImpl::initializeFromParent(const MatrixWorkspace_const_sptr parent,
  const MatrixWorkspace_sptr child, const bool differentSize) const
{
  child->setTitle(parent->getTitle());
  child->setComment(parent->getComment());
  child->setInstrument(parent->getInstrument());  // This call also copies the parameter map
  child->m_spectraMap = parent->m_spectraMap;
  child->m_sample = parent->m_sample;
  child->m_run = parent->m_run;
  child->setYUnit(parent->m_YUnit);
  child->setYUnitLabel(parent->m_YUnitLabel);
  child->isDistribution(parent->isDistribution());

  // Only copy the axes over if new sizes are not given
  if ( !differentSize )
  {
    // Only copy mask map if same size for now. Later will need to check continued validity.
    child->m_masks = parent->m_masks;
  }

  // deal with axis
  for (size_t i = 0; i < parent->m_axes.size(); ++i)
  {
    const size_t newAxisLength = child->getAxis(i)->length();
    const size_t oldAxisLength = parent->getAxis(i)->length();

    if ( !differentSize || newAxisLength == oldAxisLength )
    {
      // Need to delete the existing axis created in init above
      delete child->m_axes[i];
      // Now set to a copy of the parent workspace's axis
      child->m_axes[i] = parent->m_axes[i]->clone(child.get());
    }
    else
    {
      if (! parent->getAxis(i)->isSpectra())
      {
        if (parent->getAxis(i)->isNumeric())
        {
          Mantid::API::NumericAxis* newAxis = new Mantid::API::NumericAxis(newAxisLength);
          child->replaceAxis(i, newAxis);
          child->getAxis(i)->unit() = parent->getAxis(i)->unit();
        }
        if (parent->getAxis(i)->isText())
        {
          Mantid::API::TextAxis* newAxis = new Mantid::API::TextAxis(newAxisLength);
          child->replaceAxis(i, newAxis);
        }        
      }
      child->getAxis(i)->title() = parent->getAxis(i)->title();
    }
  }

  return;
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
MatrixWorkspace_sptr WorkspaceFactoryImpl::create(const std::string& className, const size_t& NVectors,
                                            const size_t& XLength, const size_t& YLength) const
{
  MatrixWorkspace_sptr ws;

  // Creates a managed workspace if over the trigger size and a 2D workspace is being requested.
  // Otherwise calls the vanilla create method.
  bool is2D = className.find("2D") != std::string::npos;
  bool isCompressedOK = false;
  if ( MemoryManager::Instance().goForManagedWorkspace(static_cast<size_t>(NVectors), static_cast<size_t>(XLength),
                                                          static_cast<size_t>(YLength),&isCompressedOK) && is2D )
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
      if ( static_cast<unsigned int>(blockMemory)*100/1024 > mi.availMemory )
      {
          g_log.error("There is not enough memory to allocate the workspace");
          throw std::runtime_error("There is not enough memory to allocate the workspace");
      }

      if ( !isCompressedOK )
      {
          ws = boost::dynamic_pointer_cast<MatrixWorkspace>(this->create("ManagedWorkspace2D"));
          g_log.information("Created a ManagedWorkspace2D");
      }
      else
      {
          ws = boost::dynamic_pointer_cast<MatrixWorkspace>(this->create("CompressedWorkspace2D"));
          g_log.information("Created a CompressedWorkspace2D");
      }
  }
  else
  {
      // No need for a Managed Workspace
      if ( is2D && ( className.substr(0,7) == "Managed" || className.substr(0,10) == "Compressed"))
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

 /// this create method is currently used to build MD workspaces from MD workspaces, but may be used to build MD workspaces from matrix workspaces in a future;
/*
IMDWorkspace_sptr 
WorkspaceFactoryImpl::create(const IMDWorkspace_sptr origin) const
{
    IMDWorkspace_sptr ws;
 
    ws = boost::dynamic_pointer_cast<IMDWorkspace>(this->create("MDWorkspace"));
    if (!ws)
    {
          g_log.error("MD Workspace was not created");
          throw std::runtime_error("MD Workspace was not created");
    }

    //ws->initialize(MDgeometryDescription);

    return ws;
   
}
*/

IMDWorkspace_sptr 
WorkspaceFactoryImpl::create(const std::string & className,const Geometry::MDGeometryDescription & geometryDescription) const
{
  // Avoid compiler warnings
  (void)className;
  (void)geometryDescription;

  IMDWorkspace_sptr ws;
  
  ws = boost::dynamic_pointer_cast<IMDWorkspace>(this->create("MDWorkspace"));
  if (!ws)
  {
    g_log.error("MD Workspace was not created");
    throw std::runtime_error("MD Workspace was not created");
  }
  
  // ws->initialize(geometryDescription);

  return ws;
   
}

} // namespace API
} // Namespace Mantid
