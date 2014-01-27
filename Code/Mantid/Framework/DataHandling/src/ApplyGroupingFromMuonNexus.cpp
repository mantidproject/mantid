/*WIKI*
Given a workspace or workspace group and Muon Nexus file, retrieves grouping information stored in the file and groups input workspace accordingly.
*WIKI*/

#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/ApplyGroupingFromMuonNexus.h"
#include "MantidDataObjects/Workspace2D.h"

#include <nexus/NeXusFile.hpp>
#include <boost/scoped_array.hpp>

namespace Mantid
{
namespace DataHandling
{
  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ApplyGroupingFromMuonNexus)

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ApplyGroupingFromMuonNexus::name() const { return "ApplyGroupingFromMuonNexus";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ApplyGroupingFromMuonNexus::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ApplyGroupingFromMuonNexus::category() const { return "DataHandling\\Nexus;Muon";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ApplyGroupingFromMuonNexus::initDocs()
  {
    this->setWikiSummary("Applies grouping information from Muon Nexus file to the [[workspace]].");
    this->setOptionalMessage("Applies grouping information from Muon Nexus file to the workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ApplyGroupingFromMuonNexus::init()
  {
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input), 
      "Workspace to group.");

    declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
      "Nexus file to load grouping information from." );   

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), 
      "Workspace with grouping applied.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ApplyGroupingFromMuonNexus::exec()
  {
    // Load grouping information from the Nexus file
    std::string filename = getPropertyValue("Filename");

    NeXus::File handle(filename, NXACC_READ);

    handle.openData("grouping"); // TODO: what if no?
    size_t numDetectors = static_cast<size_t>(handle.getInfo().dims[0]);

    boost::scoped_array<int> detectorGrouping(new int[numDetectors]);

    handle.getData(detectorGrouping.get());
    handle.closeData();

    Workspace_sptr inputWs = getProperty("InputWorkspace");

    std::string outputWsName = getPropertyValue("OutputWorkspace");

    if(Workspace2D_const_sptr inputWs2D = boost::dynamic_pointer_cast<const Workspace2D>(inputWs))
    {
      std::vector<int> grouping(detectorGrouping.get(), detectorGrouping.get() + numDetectors);

      Workspace2D_sptr outputWs = applyGrouping(grouping, inputWs2D);
      setProperty("OutputWorkspace", outputWs);
    }
    else if(WorkspaceGroup_const_sptr inputGroup = boost::dynamic_pointer_cast<const WorkspaceGroup>(inputWs))
    {
      WorkspaceGroup_sptr outputWsGroup = WorkspaceGroup_sptr(new WorkspaceGroup);
      
      int currentOffset = 0;

      for(size_t i = 0; i < inputGroup->size(); i++)
      {
        Workspace2D_const_sptr memberWs2D = boost::dynamic_pointer_cast<const Workspace2D>(inputGroup->getItem(i));

        if(!memberWs2D)
          throw std::invalid_argument("Specified group contains a workspace which is not a Workspace2D");

        int* from = detectorGrouping.get() + currentOffset;
        int* to = from + memberWs2D->getNumberHistograms();

        std::vector<int> grouping(from, to);

        Workspace2D_sptr outputWs = applyGrouping(grouping, memberWs2D);
        outputWsGroup->addWorkspace(outputWs);

        std::string suffix = "_" + boost::lexical_cast<std::string>(i + 1);
        std::string outWsPropName = "OutputWorkspace" + suffix;

        declareProperty(new WorkspaceProperty<Workspace>(outWsPropName, outputWsName + suffix, Direction::Output));
        setProperty(outWsPropName, boost::dynamic_pointer_cast<Workspace>(outputWs));
      }

      setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(outputWsGroup));
    }
    else
    {
      throw std::invalid_argument("Should be whether a Workspace2D or WorkspaceGroup");
    }
  }

  bool ApplyGroupingFromMuonNexus::checkGroups()
  {
    return false;
  }

  bool ApplyGroupingFromMuonNexus::processGroups()
  {
    return true;
  }

  /**
   * Applies grouping to a given workspace.
   * 
   * @param detectorGrouping :: Grouping info, where index is detector id and value is group number
   * @param          inputWs :: Workspace to group
   * @return Workspace with grouped detectors. All the unrelated parameters are left as in inputWs.
   */
 Workspace2D_sptr ApplyGroupingFromMuonNexus::applyGrouping(const std::vector<int>& detectorGrouping,
    Workspace2D_const_sptr inputWs)
  {
    if(inputWs->getNumberHistograms() != detectorGrouping.size())
        throw std::invalid_argument("Invalid number of detectors.");

    std::map< int, std::vector<detid_t> > groups;
    std::vector<detid_t> ungrouped;

    for (size_t i = 0; i < detectorGrouping.size(); ++i)
    {
      int group = detectorGrouping[i];

      if(group == 0)
        ungrouped.push_back(static_cast<detid_t>(i));
      else
        groups[group].push_back(static_cast<detid_t>(i));
    }

    if(groups.empty())
      throw std::invalid_argument("No groups specified in the input file");

    // Number of the last group we've met
    int lastGroup = groups.rbegin()->first;

    // Add ungrouped detectors to separate groups
    for(size_t i = 0; i < ungrouped.size(); i++)
      groups[++lastGroup].push_back(ungrouped[i]);

    Workspace2D_sptr groupedWs = boost::dynamic_pointer_cast<Workspace2D>(
      WorkspaceFactory::Instance().create(inputWs, groups.size(), inputWs->dataX(0).size(), inputWs->blocksize()));

    // Compile the groups
    int groupIndex = 0;
    for (auto groupIt = groups.begin(); groupIt != groups.end(); groupIt++)
    {    
      std::vector<detid_t>& detectors = groupIt->second;

      for(auto detIt = detectors.begin(); detIt != detectors.end(); detIt++)
      {
        for(size_t j = 0; j < inputWs->blocksize(); j++)
        {
          // Sum the y values
          groupedWs->dataY(groupIndex)[j] += inputWs->dataY(*detIt)[j];

          // Sum the errors in quadrature
          groupedWs->dataE(groupIndex)[j] = 
            sqrt(pow(groupedWs->dataE(groupIndex)[j], 2) + pow(inputWs->dataE(*detIt)[j], 2));
        }

        groupedWs->getSpectrum(groupIndex)->addDetectorID(static_cast<detid_t>(*detIt));
      }

      // Using the last detector X values for consistency with LoadMuonNexus1 AutoGroup behavior
      groupedWs->dataX(groupIndex) = inputWs->dataX(detectors.back());

      groupedWs->getSpectrum(groupIndex)->setSpectrumNo(groupIndex+1);

      g_log.information() << "Group " << groupIt->first << ": " 
                          << Kernel::Strings::join(detectors.begin(), detectors.end(), ", ")
                          << std::endl;

      groupIndex++;
    }

    // Set Y axis values
    for(size_t i = 0; i < groupedWs->getNumberHistograms(); i++)
      groupedWs->getAxis(1)->setValue(i, static_cast<double>(i + 1));

    return groupedWs;
  }

} // namespace DataHandling
} // namespace Mantid