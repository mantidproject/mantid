#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataHandling/SaveParameterFile.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#include <fstream>

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveParameterFile)

  using namespace Kernel;
  using namespace API;
  using namespace Geometry;

  using namespace Poco;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveParameterFile::SaveParameterFile()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveParameterFile::~SaveParameterFile()
  { }

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SaveParameterFile::name() const { return "SaveParameterFile";};

  /// Algorithm's version for identification. @see Algorithm::version
  int SaveParameterFile::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SaveParameterFile::category() const { return "DataHandling\\Instrument";}

  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveParameterFile::init()
  {
    declareProperty(new WorkspaceProperty<>("Workspace","",Direction::Input,boost::make_shared<InstrumentValidator>()),
    "Workspace to save the instrument parameters from.");

    std::vector<std::string> exts;
    exts.push_back(".xml");

    declareProperty(new API::FileProperty("Filename","", API::FileProperty::Save, exts),
    "The name of the file into which the instrument parameters will be saved.");

    declareProperty("LocationParameters", false, "Save the location parameters used to calibrate the instrument.", Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveParameterFile::exec()
  {
    const MatrixWorkspace_const_sptr ws = getProperty("Workspace");
    const bool saveLocationParams = getProperty("LocationParameters");
    const std::string filename = getProperty("Filename");

    const Instrument_const_sptr instrument = ws->getInstrument();
    const ParameterMap_sptr params = instrument->getParameterMap();

    //maps components to a tuple of parameters' name, type, and value
    std::map<ComponentID, std::vector<boost::tuple<std::string,std::string,std::string> > > toSave;

    //Set up a progress bar
    Progress prog(this, 0.0, 0.3, params->size());

    //Build a list of parameters to save;
    for(auto paramsIt = params->begin(); paramsIt != params->end(); ++paramsIt)
    {
      if(prog.hasCancellationBeenRequested())
        break;
      prog.report("Generating parameters");
      const ComponentID    cID = (*paramsIt).first;
      const std::string  pName = (*paramsIt).second->name();
      const std::string  pType = (*paramsIt).second->type();
      const std::string pValue = (*paramsIt).second->asString();

      if(pName == "x"          || pName == "y"          || pName == "z"          ||
         pName == "r-position" || pName == "t-position" || pName == "p-position" ||
         pName == "rotx"       || pName == "roty"       || pName == "rotz"        )
      {
        g_log.warning() << "The parameter name '" << pName << "' is reserved and has not been saved. "
                        << "Please contact the Mantid team for more information.";
        continue;
      }

      //If it isn't a position or rotation parameter, we can just add it to the list to save directly and move on.
      if(pName != "pos" && pName != "rot")
      {
        toSave[cID].push_back(boost::make_tuple(pName, pType, pValue));
      }
    }

    std::vector<IComponent_const_sptr> components;
    //If we're saving location parameters we'll check every component to see if its location has been changed
    if(saveLocationParams)
    {
      //Get all the components in the instrument
      instrument->getChildren(components, true);
      prog.resetNumSteps((int64_t)components.size(), 0.3, 0.6);

      for(auto cIt = components.begin(); cIt != components.end(); ++cIt)
      {
        if(prog.hasCancellationBeenRequested())
          break;
        prog.report("Generating location parameters");
        const IComponent* comp = cIt->get();
        const IComponent* baseComp = comp->getBaseComponent();
        const ComponentID cID = const_cast<ComponentID>(comp);

        //Check if the position has been changed by a parameter
        //If so, check each axis and add the relevant adjustment parameters to the to-save list.
        const V3D basePos = baseComp->getPos();
        const V3D  absPos =     comp->getPos();
        const V3D posDiff = absPos - basePos;

        const double threshold = 0.0001;

        if(std::abs(posDiff.X()) > threshold)
          toSave[cID].push_back(boost::make_tuple("x", "double", Strings::toString<double>(absPos.X())));
        if(std::abs(posDiff.Y()) > threshold)
          toSave[cID].push_back(boost::make_tuple("y", "double", Strings::toString<double>(absPos.Y())));
        if(std::abs(posDiff.Z()) > threshold)
          toSave[cID].push_back(boost::make_tuple("z", "double", Strings::toString<double>(absPos.Z())));

        //Check if the rotation has been changed by a parameter
        //If so, convert to Euler (XYZ order) and output each component that differs
        const Quat baseRot = baseComp->getRotation();
        const Quat absRot = comp->getRotation();

        if(baseRot != absRot)
        {
          //Euler rotation components are not independent so write them all out to be safe.
          std::vector<double> absEuler = absRot.getEulerAngles("XYZ");
          toSave[cID].push_back(boost::make_tuple("rotx", "double", Strings::toString<double>(absEuler[0])));
          toSave[cID].push_back(boost::make_tuple("roty", "double", Strings::toString<double>(absEuler[1])));
          toSave[cID].push_back(boost::make_tuple("rotz", "double", Strings::toString<double>(absEuler[2])));
        }
      }
    }

    //Begin writing the XML manually
    std::ofstream file(filename.c_str(), std::ofstream::trunc);
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<parameter-file instrument=\"" << instrument->getName() << "\"";
    file << " valid-from=\"" << instrument->getValidFromDate().toISO8601String() << "\">\n";

    prog.resetNumSteps((int64_t)toSave.size(), 0.6, 1.0);
    //Iterate through all the parameters we want to save and build an XML
    //document out of them.
    for(auto compIt = toSave.begin(); compIt != toSave.end(); ++compIt)
    {
      if(prog.hasCancellationBeenRequested())
        break;
      prog.report("Saving parameters");
      //Component data
      const ComponentID cID = compIt->first;
      const std::string cFullName = cID->getFullName();
      const IDetector* cDet = dynamic_cast<IDetector*>(cID);
      const detid_t cDetID = (cDet) ? cDet->getID() : 0;

      file << "	<component-link";
      if(cDetID != 0)
        file << " id=\"" << cDetID << "\"";
      file << " name=\"" << cFullName << "\">\n";
      for(auto paramIt = compIt->second.begin(); paramIt != compIt->second.end(); ++paramIt)
      {
        const std::string pName = boost::get<0>(*paramIt);
        const std::string pType = boost::get<1>(*paramIt);
        const std::string pValue = boost::get<2>(*paramIt);

        file << "		<parameter name=\"" << pName << "\"" << (pType == "string" ? " type = \"string\"" : "") << ">\n";
        file << "			<value val=\"" << pValue << "\"/>\n";
        file << "		</parameter>\n";
      }
      file << "	</component-link>\n";
    }
    file << "</parameter-file>\n";

    file.flush();
    file.close();
  }

} // namespace Algorithms
} // namespace Mantid
