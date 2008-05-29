#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/SpectraDetectorMap.h"


namespace Mantid
{
namespace API
{

Kernel::Logger& Workspace::g_log = Kernel::Logger::get("Workspace");

/// Default constructor
Workspace::Workspace() : m_axes(), m_title(), m_comment(), sptr_instrument(new Instrument),
  sptr_spectramap(new SpectraDetectorMap), m_sample(), m_history(), m_isDistribution(false)
{}

/// Destructor// RJT, 3/10/07: The Analysis Data Service needs to be able to delete workspaces, so I moved this from protected to public.
Workspace::~Workspace()
{
  for (unsigned int i = 0; i < m_axes.size(); ++i)
  {
    delete m_axes[i];
  }
}
	
/** Set the title of the workspace
 * 
 *  @param t The title
 */
void Workspace::setTitle(const std::string& t)
{
  m_title=t;
}
/** Set the comment field of the workspace
 * 
 *  @param c The comment
 */
void Workspace::setComment(const std::string& c)
{
  m_comment=c;
}
/** Set the Spectra to DetectorMap
 * 
 * \param map:: Shared pointer to the SpectraDetectorMap
 */
void Workspace::setSpectraMap(const boost::shared_ptr<SpectraDetectorMap>& map)
{
	sptr_spectramap=map;
}
/** Set the instrument
 * 
 * \param instr :: Shared pointer to an instrument instrument.
 */
void Workspace::setInstrument(const boost::shared_ptr<Instrument>& instr)
{
	sptr_instrument=instr;
}

/** Get the workspace title
 * 
 *  @return The title
 */
const std::string& Workspace::getTitle() const
{
  return m_title;
}

/** Get the workspace comment
 * 
 *  @return The comment
 */
const std::string& Workspace::getComment() const
{
  return m_comment;
}

/** Get a shared pointer to the SpectraDetectorMap associated with this workspace
 * 
 *  @return The SpectraDetectorMap
 */
boost::shared_ptr<SpectraDetectorMap> Workspace::getSpectraMap() const
{
  return sptr_spectramap;
}

/** Get a shared pointer to the instrument associated with this workspace
 * 
 *  @return The instrument class
 */
boost::shared_ptr<Instrument> Workspace::getInstrument() const
{
  return sptr_instrument;
}

/** Get the sample associated with this workspace
 * 
 *  @return The sample class
 */
Sample& Workspace::getSample()
{
  return m_sample;
}

/** Get a pointer to a workspace axis
 *  @param axisIndex The index of the axis required
 *  @throw IndexError If the argument given is outside the range of axes held by this workspace
 */
Axis* const Workspace::getAxis(const int axisIndex) const
{
  if ( axisIndex < 0 || axisIndex >= static_cast<int>(m_axes.size()) )
  {
    throw Kernel::Exception::IndexError(axisIndex, m_axes.size(),"Argument to getAxis is invalid for this workspace");
  }
  
  return m_axes[axisIndex];
}

/// Are the Y-values in this workspace dimensioned?
const bool& Workspace::isDistribution() const
{
  return m_isDistribution;
}

/// Set the flag for whether the Y-values are dimensioned
bool& Workspace::isDistribution(bool newValue)
{
  m_isDistribution = newValue;
  return m_isDistribution;
}

} // namespace API
} // Namespace Mantid


///\cond TEMPLATE
template DLLExport class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef,Mantid::API::Workspace>;
template DLLExport class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::API::Workspace>;

template DLLExport class Mantid::API::WorkspaceProperty<Mantid::API::Workspace>;

namespace Mantid
{
namespace Kernel
{
template<> DLLExport
Mantid::API::Workspace_sptr PropertyManager::getValue<Mantid::API::Workspace_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::Workspace_sptr>* prop = 
                    dynamic_cast<PropertyWithValue<Mantid::API::Workspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    throw std::runtime_error("Attempt to assign property of incorrect type");
  }
}
} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
