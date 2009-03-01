//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/detail/classification.hpp>
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/IInstrument.h"
#include <queue>
#include <fstream>
#include "MantidAlgorithms/CreateCalFileByNames.h"


namespace Mantid
{
namespace Algorithms
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CreateCalFileByNames)

using namespace Kernel;
using API::WorkspaceProperty;
using API::MatrixWorkspace_sptr;
using API::MatrixWorkspace;

// Get a reference to the logger
Logger& CreateCalFileByNames::g_log = Logger::get("CreateCalFileByNames");

CreateCalFileByNames::CreateCalFileByNames():API::Algorithm(),group_no(0)
{
}
/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void CreateCalFileByNames::init()
{
  declareProperty("InstrumentName","");
  declareProperty("GroupingFileName","",new FileValidator(std::vector<std::string>(1,"cal"),false));
  declareProperty("GroupNames","");
}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw runtime_error If unable to run one of the sub-algorithms successfully
 */
void CreateCalFileByNames::exec()
{
	std::ostringstream mess;
	// Check that the instrument is in store
	std::string instname=getProperty("InstrumentName");
	std::string instshort=instname.substr(0,3);
	std::transform(instshort.begin(),instshort.end(),instshort.begin(),toupper);
	instshort=instshort+"_Definition.xml";
	if (!API::InstrumentDataService::Instance().doesExist(instshort))
	{
		g_log.error("Instrument "+instshort+" is not present in data store.");
		throw std::runtime_error("Instrument "+instshort+" is not present in data store.");
	}
	// Get the instrument.
	API::IInstrument_sptr inst=API::InstrumentDataService::Instance().retrieve(instshort);

	// Get the names of groups
	std::string groupsname=getProperty("GroupNames");
	std::vector<std::string> groups;
	boost::split( groups, groupsname, boost::algorithm::detail::is_any_ofF<char>(",/"));
	if (groups.empty())
	{
		g_log.error("Could not determine group names. Group names should be separated by / or ,");
		throw std::runtime_error("Could not determine group names. Group names should be separated by / or ,");
	}
	// Assign incremental number to each group
	std::map<std::string,int> group_map;
	int index=0;
	for (std::vector<std::string>::const_iterator it=groups.begin();it!=groups.end();it++)
		group_map[(*it)]=++index;

	// Find Detectors that belong to groups
	typedef boost::shared_ptr<Geometry::ICompAssembly> sptr_ICompAss;
	typedef boost::shared_ptr<Geometry::IComponent> sptr_IComp;
	typedef boost::shared_ptr<Geometry::IDetector> sptr_IDet;
	std::queue< std::pair<sptr_ICompAss,int> > assemblies;
	sptr_ICompAss current=boost::dynamic_pointer_cast<Geometry::ICompAssembly>(inst);
	sptr_IDet currentDet;
	sptr_IComp currentIComp;
	sptr_ICompAss currentchild;

	int top_group, child_group;

	if (current.get())
	{
		top_group=group_map[current->getName()]; // Return 0 if not in map
		assemblies.push(std::make_pair<sptr_ICompAss,int>(current,top_group));
	}
	std::string filename=getProperty("GroupingFilename");
	std::ofstream file(filename.c_str());

	file << "# Grouping file for instrument "+instshort+" created by Mantid \n";
	file << "# Created using grouping assemblies:" << groupsname << "\n";
	file << "# Format: number  UDET offset  select  group \n";
	int entries=0;
	while(!assemblies.empty()) //Travel the tree from the instrument point
	{
		current=assemblies.front().first;
		top_group=assemblies.front().second;
		assemblies.pop();
		int nchilds=current->nelements();
		if (nchilds!=0)
		{
			for (int i=0;i<nchilds;++i)
			{
				currentIComp=(*(current.get()))[i]; // Get child
				currentDet=boost::dynamic_pointer_cast<Geometry::IDetector>(currentIComp);
				if (currentDet.get())// Is detector
				{
					file << entries++ << " " << currentDet->getID() << " " << "0.00000 " << "1 " << top_group << "\n";
				}
				else // Is an assembly, push in the queue
				{
					currentchild=boost::dynamic_pointer_cast<Geometry::ICompAssembly>(currentIComp);
					if (currentchild.get())
					{
						child_group=group_map[currentchild->getName()];
						if (child_group==0)
							child_group=top_group;
						assemblies.push(std::make_pair<sptr_ICompAss,int>(currentchild,child_group));
					}
				}
			}
		}
	}
	return;
}



} // namespace Algorithm
} // namespace Mantid
