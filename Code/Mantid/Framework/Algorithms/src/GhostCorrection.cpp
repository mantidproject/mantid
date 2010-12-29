//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GhostCorrection.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/BinaryFile.h"
#include "MantidKernel/BinFinder.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/AlignDetectors.h"

namespace Mantid
{
  using namespace Mantid;
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(GhostCorrection)

    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;
    using std::string;

    //Ghost pixels per input pixel. Should this be a parameter?
    const int NUM_GHOSTS = 16;


    //=============================================================================
    /// Constructor
    GhostCorrection::GhostCorrection() : API::Algorithm()
    {
      this->tof_to_d = NULL;
      this->groupedGhostMaps.clear();
      this->rawGhostMap = NULL;
      this->input_detectorIDToWorkspaceIndexMap = NULL;
    }

    //=============================================================================
    /// Destructor
    GhostCorrection::~GhostCorrection()
    {
      delete this->tof_to_d;
      delete this->rawGhostMap;
      delete this->input_detectorIDToWorkspaceIndexMap;
      for (size_t i=0; i < this->groupedGhostMaps.size(); i++)
        delete this->groupedGhostMaps[i];
    }

    //=============================================================================
    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void GhostCorrection::init()
    {
      nGroups = 0;

      //Input workspace must be in dSpacing and be an EventWorkspace
      API::CompositeValidator<MatrixWorkspace> *wsValidator = new API::CompositeValidator<MatrixWorkspace>;
      wsValidator->add(new API::WorkspaceUnitValidator<MatrixWorkspace>("TOF"));
      wsValidator->add(new API::RawCountValidator<MatrixWorkspace>);
      wsValidator->add(new API::EventWorkspaceValidator<MatrixWorkspace>);

      declareProperty(
        new WorkspaceProperty<>("InputWorkspace", "",Direction::Input, wsValidator),
        "EventWorkspace from which to make a ghost correction histogram.");

      declareProperty(
        new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
        "The name to give the output workspace; it will be a Workspace2D");

      declareProperty(
        new ArrayProperty<double>("BinParams", new RebinParamsValidator),
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be followed by a comma and more widths and last boundary pairs.\n"
        "Negative width values indicate logarithmic binning.");

      declareProperty(new FileProperty("GroupingFilename", "", FileProperty::Load, ".cal"),
          "The name of the CalFile with grouping data" );

      declareProperty(new FileProperty("GhostCorrectionFilename", "", FileProperty::Load, "dat"),
          "The name of the file containing the ghost correction mapping." );
//
//      declareProperty(
//        new PropertyWithValue<bool>("UseParallelAlgorithm", false),
//        "Check to use the parallelized algorithm; unchecked uses a simpler direct one.");

    }



    //=============================================================================
    /** Reads in the file with the grouping information
     * @param groupingFilename The file that contains the group information
     * TODO: Abstract this out into its own class, to reuse code better.
     */
    void GhostCorrection::readGroupingFile(const std::string& groupingFilename)
    {
      std::ifstream grFile(groupingFilename.c_str());
      if (!grFile.is_open())
      {
        g_log.error() << "Unable to open grouping file " << groupingFilename << std::endl;
        throw Exception::FileError("Error reading .cal file",groupingFilename);
      }

      int maxGroup = -1;

      detId_to_group.clear();
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
          //Save the group of this detector ID
          detId_to_group[udet]=group;
          //Save the offset
          detId_to_offset[udet]=offset;
          //Count the groups
          if (group > maxGroup)
            maxGroup = group;
        }
      }
      grFile.close();

      //Count the # of groups this way.
      nGroups = maxGroup+1;

      return;
    }


    //=============================================================================
    /**
     * Loads a ghost mapping file from disk. Does a lot of the
     * work of mapping what ghost pixels should go were.
     * this->inputW needs to be set first!
     * @param ghostMapFile the filename containing the ghost mapping
     */
    void GhostCorrection::loadGhostMap(std::string ghostMapFile)
    {
      //Open the file
      BinaryFile<GhostDestinationValue> ghostFile(ghostMapFile);

      //Load all the ghost corrections
      rawGhostMap = ghostFile.loadAll();

      if (rawGhostMap->size() % NUM_GHOSTS != 0)
      {
        delete rawGhostMap; rawGhostMap=NULL;
        throw std::runtime_error("The ghost correction file specified is not of the expected size.");
      }

      //Prepare the groups' ghost sources lists
      this->groupedGhostMaps.clear();
      for (int i=0; i < this->nGroups; i++)
        this->groupedGhostMaps.push_back( new GhostSourcesMap() );

      size_t numInPixels = rawGhostMap->size() / NUM_GHOSTS;
      size_t fileIndex = 0;

      for (size_t inPixelId = 0; inPixelId < numInPixels; inPixelId++)
      {
        //Find the input workspace index corresponding to this input Pixel ID
        IndexToIndexMap::iterator it;
        it = input_detectorIDToWorkspaceIndexMap->find(inPixelId);

        if (it != input_detectorIDToWorkspaceIndexMap->end())
        {
          //A valid workspace index was found, this one:
          int inputWorkspaceIndex = it->second;

          //To check for sameness of group in all ghosts
          int lastGroup = -1;
          int ghostGroup = -1;
          bool allSameGroup = true;

          for (int g=0; g < NUM_GHOSTS; g++)
          {
            //Calculate the index into the file
            fileIndex = inPixelId * NUM_GHOSTS + g;

            //This is where the ghost ends up
            GhostDestinationValue ghostVal = (*rawGhostMap)[fileIndex];
            size_t ghostPixelId = ghostVal.pixelId;

            //Don't do anything if the weight is <0
            if (ghostVal.weight > 0.0)
            {
              //Find the group # for this ghost pixel id
              ghostGroup = this->detId_to_group[ghostPixelId];
              //Check for changing group #
              if (lastGroup == -1)
                lastGroup = ghostGroup;
              else
              {
                if (lastGroup != ghostGroup)
                {
                  //Unsupported case of ghosts going in different groups.
                  allSameGroup = false;
                  break;
                }
              }
            } //non-zero weight

          } //for each ghost

          if (ghostGroup >-1)
          {
            //At least one of the ghosts has non-zero weight
            if (allSameGroup)
            {
              //Ghosts belong to same group

              //And get the list of sources from it.
              GhostSourcesMap * thisGroupsGhostMap = this->groupedGhostMaps[ghostGroup];

              //Look if the inputWorkspaceIndex is already in this list.
              GhostSourcesMap::iterator gsmit = thisGroupsGhostMap->find(inputWorkspaceIndex);
              if (gsmit == thisGroupsGhostMap->end())
              {
                //That workspace index was not in the map before. This is normal
                //Add the entry, save the pixel ID for later
                (*thisGroupsGhostMap)[inputWorkspaceIndex] = inPixelId;
              }
              else
                //Input workspace is already there?
                g_log.warning() << "GhostCorrection: input WorkspaceIndex " << inputWorkspaceIndex << " was found more than once in group "
                                << ghostGroup << ". This should not happen. Ghost is ignored.\n";


            } //same group
            else
              g_log.warning() << "GhostCorrection: input WorkspaceIndex " << inputWorkspaceIndex << " causes ghosts in separate groups. "
                              << "This is not supported, and the ghosts from this pixel will be ignored.\n";

          } //there are non-zero weights!


        } //(valid WS index)
      }

    }


    //=============================================================================
    /** Execute the ghost correction on all events in the input workspace **/
    void GhostCorrection::exec()
    {

      // Get the input workspace
      this->inputW = getProperty("InputWorkspace");

      //Now, determine if the input workspace is actually an EventWorkspace
      EventWorkspace_const_sptr eventW = boost::dynamic_pointer_cast<const EventWorkspace>(inputW);
      if (eventW == NULL)
        throw std::runtime_error("Invalid workspace type provided to GhostCorrection. Only EventWorkspaces work with this algorithm.");

      //Load the grouping (and offsets) file
      this->readGroupingFile( getProperty("GroupingFilename") );
      if (this->nGroups <= 0)
        throw std::runtime_error("The # of groups found in the Grouping file is 0.");

      //Make the X axis to bin to.
      MantidVecPtr XValues_new;
      const int numbins = VectorHelper::createAxisFromRebinParams(getProperty("BinParams"), XValues_new.access());

      //Prepare the binfinder class
      BinFinder binner( getProperty("BinParams") );
      if (binner.lastBinIndex() != static_cast<int>(XValues_new.access().size()-1))
      {
        std::stringstream msg;
        msg << "GhostCorrection: The binner found " << binner.lastBinIndex()+1 << " bins, but the X axis has "
            << XValues_new.access().size() << ". Try different binning parameters.";
        throw std::runtime_error(msg.str());
      }

      //Create an output Workspace2D with group # of output spectra
      MatrixWorkspace_sptr outputW = WorkspaceFactory::Instance().create("Workspace2D", this->nGroups-1, numbins, numbins-1);
      WorkspaceFactory::Instance().initializeFromParent(inputW, outputW,  true);

      //Set the X bins in the output WS.
      Workspace2D_sptr outputWS2D = boost::dynamic_pointer_cast<Workspace2D>(outputW);
      for (int i=0; i < outputWS2D->getNumberHistograms(); i++)
        outputWS2D->setX(i, XValues_new);

      //Prepare the maps you need
      input_detectorIDToWorkspaceIndexMap = inputW->getDetectorIDToWorkspaceIndexMap(true);

      //Load the ghostmapping file
      this->loadGhostMap( getProperty("GhostCorrectionFilename") );

      //Initialize progress reporting.
      int numsteps = 0; //count how many steps
      for (int gr=1; gr < this->nGroups; ++gr)
        numsteps += this->groupedGhostMaps[gr]->size();
      Progress prog(this, 0.0, 1.0, numsteps);

      //Set up the tof-to-d_spacing map for all pixel ids.
      this->tof_to_d = Mantid::Algorithms::calcTofToD_ConversionMap(inputW, this->detId_to_offset);

      // Set the final unit that our output workspace will have
      outputW->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

      //Go through the groups, starting at #1!
      PARALLEL_FOR2(eventW, outputW)
      for (int gr=1; gr < this->nGroups; ++gr)
      {
        PARALLEL_START_INTERUPT_REGION

        //TODO: Convert between group # and workspace index. Sigh.
        //Groups normally start at 1 and so the workspace index will be one below that.
        int outputWorkspaceIndex = gr-1;

        //Start by making sure the Y and E values are 0.
        MantidVec& Y = outputW->dataY(outputWorkspaceIndex);
        MantidVec& E = outputW->dataE(outputWorkspaceIndex);
        Y.assign(Y.size(), 0.0);
        E.assign(E.size(), 0.0);

        //Perform the GhostCorrection

        //Ok, this map has as keys the source workspace indices
        GhostSourcesMap * thisGroupsGhostMap = this->groupedGhostMaps[gr];
        GhostSourcesMap::iterator it;
        for (it = thisGroupsGhostMap->begin(); it != thisGroupsGhostMap->end(); it++)
        {
          //This workspace index is causing 16 ghosts in this group.
          int inputWorkspaceIndex = it->first;
          int inputDetectorID = it->second;

          //This is the events in the pixel CAUSING the ghost.
          const EventList & sourceEventList = eventW->getEventList(inputWorkspaceIndex);

          //Now get the actual vector of tofevents
          const std::vector<TofEvent>& events = sourceEventList.getEvents();
          size_t numEvents = events.size();

          //Go through all events.
          for (size_t i=0; i < numEvents; i++)
          {
            const TofEvent& event = events[i];

            for (int g=0; g < NUM_GHOSTS; g++)
            {
              //Find the ghost correction
              int fileIndex = inputDetectorID * NUM_GHOSTS + g;
              GhostDestinationValue ghostVal = (*rawGhostMap)[fileIndex];

              //Convert to d-spacing using the factor of the GHOST pixel id
              double d_spacing = event.tof() * (*tof_to_d)[ghostVal.pixelId];

              //Find the bin for this d-spacing
              int binIndex = binner.bin(d_spacing);

              if (binIndex >= 0)
              {
                //Slap it in the Y array for this group; use the weight.
                Y[binIndex] += ghostVal.weight;
              }

            } //for each ghost

          } //for each event


          //Report progress
          prog.report();
        }


        PARALLEL_END_INTERUPT_REGION
      }

      PARALLEL_CHECK_INTERUPT_REGION

      // Assign the workspace to the output workspace property
      setProperty("OutputWorkspace", outputW);


    }





  } // namespace Algorithm
} // namespace Mantid
