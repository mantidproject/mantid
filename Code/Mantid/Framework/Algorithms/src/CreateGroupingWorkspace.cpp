#include "MantidAlgorithms/CreateGroupingWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <queue>
#include <fstream>
#include "MantidAPI/FileProperty.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(CreateGroupingWorkspace)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::Geometry;
  using namespace Mantid::DataObjects;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateGroupingWorkspace::CreateGroupingWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateGroupingWorkspace::~CreateGroupingWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void CreateGroupingWorkspace::initDocs()
  {
    this->setWikiSummary("Creates a new GroupingWorkspace using an instrument from one of: an input workspace, an instrument name, or an instrument IDF file.\nOptionally uses bank names to create the groups.");
    this->setOptionalMessage("Creates a new GroupingWorkspace using an instrument from one of: an input workspace, an instrument name, or an instrument IDF file.\Optionally uses bank names to create the groups.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void CreateGroupingWorkspace::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, true),
        "Optional: An input workspace with the instrument we want to use.");

    declareProperty(new PropertyWithValue<std::string>("InstrumentName","",Direction::Input),
        "Optional: Name of the instrument to base the GroupingWorkspace on which to base the GroupingWorkspace.");

    declareProperty(new FileProperty("InstrumentFilename", "", FileProperty::OptionalLoad, ".xml"),
        "Optional: Path to the instrument definition file on which to base the GroupingWorkspace.");

    declareProperty(new FileProperty("OldCalFilename", "", FileProperty::OptionalLoad, ".cal"),
        "Optional: Path to the old-style .cal grouping/calibration file (multi-column ASCII). You must also specify the instrument.");

    declareProperty("GroupNames","",
      "Optional: A string of the instrument component names to use as separate groups.\n"
      "Use / or , to separate multiple groups.\n"
      "If empty, then an empty GroupingWorkspace will be created.");

    declareProperty(new WorkspaceProperty<GroupingWorkspace>("OutputWorkspace","",Direction::Output),
        "An output GroupingWorkspace.");
  }


  //------------------------------------------------------------------------------------------------
  /** Read old-style .cal file to get the grouping
   *
   * @param groupingFileName :: path to .cal multi-col ascii
   * @param detIDtoGroup :: map of key=detectorID, value=group number.
   * @param prog :: progress reporter
   */
  void readGroupingFile(const std::string& groupingFileName, std::map<detid_t, int> & detIDtoGroup, Progress & prog )
  {
    std::ifstream grFile(groupingFileName.c_str());
    if (!grFile.is_open())
    {
      throw Exception::FileError("Error reading .cal file",groupingFileName);
    }
    detIDtoGroup.clear();
    std::string str;
    while(getline(grFile,str))
    {
      //Comment
      if (str.empty() || str[0] == '#') continue;
      std::istringstream istr(str);
      int n,udet,sel,group;
      double offset;
      istr >> n >> udet >> offset >> sel >> group;
      if ((sel) && (group>0))
      {
        detIDtoGroup[udet]=group; //Register this detector id
      }
      prog.report();
    }
    grFile.close();
    return;
  }


  //------------------------------------------------------------------------------------------------
  /** Use bank names to build grouping
   *
   * @param GroupNames :: comma-sep list of bank names
   * @param inst :: instrument
   * @param detIDtoGroup :: output: map of detID: to group number
   * @param prog :: progress report
   */
  void makeGroupingByNames(std::string GroupNames, IInstrument_sptr inst, std::map<detid_t, int> & detIDtoGroup, Progress & prog)
  {
    // Split the names of the group and insert in a vector
    std::vector<std::string> vgroups;
    boost::split( vgroups, GroupNames, boost::algorithm::detail::is_any_ofF<char>(",/*"));

    // Assign incremental number to each group
    std::map<std::string,int> group_map;
    int index=0;
    for (std::vector<std::string>::const_iterator it=vgroups.begin();it!=vgroups.end();it++)
      group_map[(*it)]=++index;

    // Find Detectors that belong to groups
    if (group_map.size() > 0)
    {
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

      prog.setNumSteps( int(assemblies.size()) );

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
              if (top_group > 0)
              {
                detIDtoGroup[currentDet->getID()] = top_group;
              }
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
        prog.report();
      }

      return;
    }
  }



  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void CreateGroupingWorkspace::exec()
  {
    MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
    std::string InputWorkspace = getPropertyValue("InputWorkspace");
    std::string InstrumentName = getPropertyValue("InstrumentName");
    std::string InstrumentFilename = getPropertyValue("InstrumentFilename");
    std::string OldCalFilename = getPropertyValue("OldCalFilename");
    std::string GroupNames = getPropertyValue("GroupNames");

    // Some validation
    int numParams = 0;
    if (inWS) numParams++;
    if (!InstrumentName.empty()) numParams++;
    if (!InstrumentFilename.empty()) numParams++;

    if (numParams > 1)
      throw std::invalid_argument("You must specify exactly ONE way to get an instrument (workspace, instrument name, or IDF file). You specified more than one.");
    if (numParams == 0)
      throw std::invalid_argument("You must specify exactly ONE way to get an instrument (workspace, instrument name, or IDF file). You specified none.");

    if (!OldCalFilename.empty() && !GroupNames.empty())
      throw std::invalid_argument("You must specify either to use the OldCalFilename parameter OR GroupNames but not both!");

    // ---------- Get the instrument one of 3 ways ---------------------------
    IInstrument_sptr inst;
    if (inWS)
    {
      inst = inWS->getInstrument();
    }
    else
    {
      Algorithm_sptr childAlg = createSubAlgorithm("LoadInstrument",0.0,0.2);
      MatrixWorkspace_sptr tempWS(new Workspace2D());
      childAlg->setProperty<MatrixWorkspace_sptr>("Workspace", tempWS);
      childAlg->setPropertyValue("Filename", InstrumentFilename);
      childAlg->setPropertyValue("InstrumentName", InstrumentName);
      childAlg->executeAsSubAlg();
      inst = tempWS->getInstrument();
    }


    // --------------------------- Create the output --------------------------
    GroupingWorkspace_sptr outWS(new GroupingWorkspace(inst));
    this->setProperty("OutputWorkspace", outWS);

    // This will get the grouping
    std::map<detid_t, int> detIDtoGroup;

    Progress prog(this,0.2,1.0, outWS->getNumberHistograms() );

    // Make the grouping one of two ways:
    if (GroupNames != "")
      makeGroupingByNames(GroupNames, inst, detIDtoGroup, prog);
    else if (OldCalFilename != "")
      readGroupingFile(OldCalFilename, detIDtoGroup, prog);

    g_log.information() << detIDtoGroup.size() << " entries in the detectorID-to-group map.\n";



    if (detIDtoGroup.size() != 0)
    {
      size_t numNotFound = 0;

      // Make the groups, if any
      std::map<detid_t, int>::const_iterator it_end = detIDtoGroup.end();
      std::map<detid_t, int>::const_iterator it;
      for (it = detIDtoGroup.begin(); it != it_end; it++)
      {
        int detID = it->first;
        int group = it->second;
        try
        {
          outWS->setValue(detID, double(group));
        }
        catch (std::invalid_argument & e)
        {
          numNotFound++;
        }
      }

      if (numNotFound > 0)
        g_log.warning() << numNotFound << " detector IDs (out of " << detIDtoGroup.size() << ") were not found in the instrument\n.";

    }

  }




} // namespace Mantid
} // namespace Algorithms

