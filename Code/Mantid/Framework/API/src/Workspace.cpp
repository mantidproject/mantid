#include "MantidAPI/Workspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

#include <boost/lexical_cast.hpp>

namespace Mantid
{
namespace API
{

/// Default constructor
Workspace::Workspace()
: DataItem(),
  m_title(), m_comment(), m_name(), m_history()
{}

/** Copy constructor
 * @param other :: workspace to copy
 */
Workspace::Workspace(const Workspace & other)
: DataItem(other),
  m_title(other.m_title), m_comment(other.m_comment), m_name(other.m_name), m_history(other.m_history)
{
}


/// Workspace destructor
Workspace::~Workspace()
{
}


/** Set the title of the workspace
 *
 *  @param t :: The title
 */
void Workspace::setTitle(const std::string& t)
{
  m_title=t;
}

/** Set the comment field of the workspace
 *
 *  @param c :: The comment
 */
void Workspace::setComment(const std::string& c)
{
  m_comment=c;
}

/** Set the name field of the workspace
 *
 *  @param name :: The name
 */
void Workspace::setName(const std::string& name)
{
  m_name = name;
}

/** Get the workspace title
 *
 *  @return The title
 */
const std::string Workspace::getTitle() const
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

/** Get the workspace name
 *
 *  @return The name
 */
const std::string& Workspace::getName() const
{
  return m_name;
}

/**
 * Check whether other algorithms have been applied to the
 * workspace by checking the history length.
 *
 * By default a workspace is called dirty if its history is
 * longer than one. This default can be changed to allow for
 * workspace creation processes that necessitate more than
 * a single algorithm.
 *
 * @param n: number of algorithms defining a clean workspace
 */
bool Workspace::isDirty(const int n) const
{
    return static_cast<int>(m_history.size()) > n;
}

/**
 * Create this workspace's InfoNode and add it to a parent node.
 * @param parentNode :: A node to add to.
 */
void Workspace::addInfoNodeTo(Workspace::InfoNode &parentNode) const
{
    InfoNode *node = createInfoNode();
    parentNode.addNode( node );
}

/**
 * @return A pointer to a created InfoNode.
 */
Workspace::InfoNode *Workspace::createInfoNode() const
{
    return new InfoNode(*this);
}

/**
 * Construct an empty instance of InfoNode. Intended to be used by the ADS.
 */
Workspace::InfoNode::InfoNode(const AnalysisDataServiceImpl *)
    :m_icon(Default)
{
}

/**
 * Construct an instance of InfoNode.
 * @param workspace :: The workspace for which this info is created. InfoNode
 * extracts the workspace name and its memory size and adds them to the info.
 */
Workspace::InfoNode::InfoNode(const Workspace &workspace)
{
    m_info.push_back( workspace.id() );
    m_workspaceName = workspace.name();
    m_memorySize = workspace.getMemorySize();
    m_icon = Default;
    if ( dynamic_cast<const ITableWorkspace*>(&workspace) )
    {
        m_icon = Table;
    }
    else if ( dynamic_cast<const MatrixWorkspace*>(&workspace) )
    {
        m_icon = Matrix;
    }
    else if ( dynamic_cast<const IMDWorkspace*>(&workspace) )
    {
        m_icon = MD;
    }
    else if ( dynamic_cast<const WorkspaceGroup*>(&workspace) )
    {
        m_icon = Group;
    }
}

Workspace::InfoNode::~InfoNode()
{
    for(auto node = m_nodes.begin(); node != m_nodes.end(); ++node)
    {
        delete *node;
    }
}

/**
 * Add a new line.
 * @param line :: A line of info to add.
 */
void Workspace::InfoNode::addLine(const std::string &line)
{
    m_info.push_back( line );
}

/**
 * Add a new info node.
 * @param node :: A pointer to a new InfoNode. The parent node takes ownership
 * of it and is responsible for its deletion.
 */
void Workspace::InfoNode::addNode(Workspace::InfoNode *node)
{
    m_nodes.push_back( node );
}

/**
 * @param workspace :: Workspace to get info for.
 */
void Workspace::InfoNode::addExperimentInfo(const Workspace &workspace)
{
    auto expInfo = dynamic_cast<const ExperimentInfo*>( &workspace );
    if ( expInfo )
    {
        std::ostringstream out;

        Geometry::Instrument_const_sptr inst = expInfo->getInstrument();
        out << "Instrument: " << inst->getName() << " ("
            << inst->getValidFromDate().toFormattedString("%Y-%b-%d")
            << " to " << inst->getValidToDate().toFormattedString("%Y-%b-%d") << ")";

        addLine( out.str() );
        if (expInfo->sample().hasOrientedLattice())
        {
          const Geometry::OrientedLattice & latt = expInfo->sample().getOrientedLattice();
          out.str("");
          out << "Sample: a " << std::fixed << std::setprecision(1) << latt.a() <<", b " << latt.b() << ", c " << latt.c();
          out << "; alpha " << std::fixed << std::setprecision(0) << latt.alpha() <<", beta " << latt.beta() << ", gamma " << latt.gamma();
          addLine( out.str() );
        }
    }
}

} // namespace API
} // Namespace Mantid

///\cond TEMPLATE
namespace Mantid
{
namespace Kernel
{

template<> MANTID_API_DLL
Mantid::API::Workspace_sptr IPropertyManager::getValue<Mantid::API::Workspace_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::Workspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::Workspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected Workspace.";
    throw std::runtime_error(message);
  }
}

template<> MANTID_API_DLL
Mantid::API::Workspace_const_sptr IPropertyManager::getValue<Mantid::API::Workspace_const_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::Workspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::Workspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return prop->operator()();
  }
  else
  {
    std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const Workspace.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
///\endcond TEMPLATE

