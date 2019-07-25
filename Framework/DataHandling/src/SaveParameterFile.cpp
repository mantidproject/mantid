// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveParameterFile.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#include <fstream>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveParameterFile)

using namespace Kernel;
using namespace API;
using namespace Geometry;

using namespace Poco;

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SaveParameterFile::name() const {
  return "SaveParameterFile";
}

/// Algorithm's version for identification. @see Algorithm::version
int SaveParameterFile::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveParameterFile::category() const {
  return "DataHandling\\Instrument";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveParameterFile::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "Workspace", "", Direction::Input,
                      boost::make_shared<InstrumentValidator>()),
                  "Workspace to save the instrument parameters from.");

  declareProperty(
      std::make_unique<API::FileProperty>("Filename", "",
                                          API::FileProperty::Save, ".xml"),
      "The name of the file into which the instrument parameters will be "
      "saved.");

  declareProperty(
      "LocationParameters", false,
      "Save the location parameters used to calibrate the instrument.",
      Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveParameterFile::exec() {
  const MatrixWorkspace_const_sptr ws = getProperty("Workspace");
  const bool saveLocationParams = getProperty("LocationParameters");
  const std::string filename = getProperty("Filename");

  const Instrument_const_sptr instrument = ws->getInstrument();
  // Create legacy parameter map with positions and other parameters extracted
  // from DetectorInfo.
  const ParameterMap_sptr params = instrument->makeLegacyParameterMap();

  // maps components to a tuple of parameters' name, type, and value
  std::map<ComponentID,
           std::vector<boost::tuple<std::string, std::string, std::string>>>
      toSave;

  // Set up a progress bar
  Progress prog(this, 0.0, 0.3, params->size());

  // Build a list of parameters to save;
  for (auto &paramsIt : *params) {
    if (prog.hasCancellationBeenRequested())
      break;
    prog.report("Generating parameters");
    const ComponentID cID = paramsIt.first;
    const std::string pName = paramsIt.second->name();
    const std::string pType = paramsIt.second->type();
    const std::string pValue = paramsIt.second->asString();

    if (pName == "x" || pName == "y" || pName == "z" || pName == "r-position" ||
        pName == "t-position" || pName == "p-position" || pName == "rotx" ||
        pName == "roty" || pName == "rotz") {
      g_log.warning() << "The parameter name '" << pName
                      << "' is reserved and has not been saved. "
                      << "Please contact the Mantid team for more information.";
      continue;
    }

    if (pName == "pos") {
      if (saveLocationParams) {
        V3D pos;
        std::istringstream pValueSS(pValue);
        pos.readPrinted(pValueSS);
        toSave[cID].push_back(boost::make_tuple(
            "x", "double", boost::lexical_cast<std::string>(pos.X())));
        toSave[cID].push_back(boost::make_tuple(
            "y", "double", boost::lexical_cast<std::string>(pos.Y())));
        toSave[cID].push_back(boost::make_tuple(
            "z", "double", boost::lexical_cast<std::string>(pos.Z())));
      }
    } else if (pName == "rot") {
      if (saveLocationParams) {
        V3D rot;
        std::istringstream pValueSS(pValue);
        rot.readPrinted(pValueSS);
        toSave[cID].push_back(boost::make_tuple(
            "rotx", "double", boost::lexical_cast<std::string>(rot.X())));
        toSave[cID].push_back(boost::make_tuple(
            "roty", "double", boost::lexical_cast<std::string>(rot.Y())));
        toSave[cID].push_back(boost::make_tuple(
            "rotz", "double", boost::lexical_cast<std::string>(rot.Z())));
      }
    }
    // If it isn't a position or rotation parameter, we can just add it to the
    // list to save directly and move on.
    else {
      if (pType == "fitting") {
        // With fitting parameters we do something special (i.e. silly)
        // We create an entire XML element to be inserted into the output,
        // instead of just giving a single fixed value
        const auto &fitParam = paramsIt.second->value<FitParameter>();
        const std::string fpName =
            fitParam.getFunction() + ":" + fitParam.getName();
        std::stringstream fpValue;
        fpValue << "<formula";
        fpValue << " eq=\"" << fitParam.getFormula() << "\"";
        fpValue << " unit=\"" << fitParam.getFormulaUnit() << "\"";
        fpValue << " result-unit=\"" << fitParam.getResultUnit() << "\"";
        fpValue << "/>";
        toSave[cID].push_back(
            boost::make_tuple(fpName, "fitting", fpValue.str()));
      } else
        toSave[cID].push_back(boost::make_tuple(pName, pType, pValue));
    }
  }

  // Begin writing the XML manually
  std::ofstream file(filename.c_str(), std::ofstream::trunc);
  file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  file << "<parameter-file instrument=\"" << instrument->getName() << "\"";
  file << " valid-from=\"" << instrument->getValidFromDate().toISO8601String()
       << "\">\n";

  prog.resetNumSteps(static_cast<int64_t>(toSave.size()), 0.6, 1.0);
  // Iterate through all the parameters we want to save and build an XML
  // document out of them.
  for (const auto &comp : toSave) {
    if (prog.hasCancellationBeenRequested())
      break;
    prog.report("Saving parameters");
    // Component data
    const ComponentID cID = comp.first;
    const std::string cFullName = cID->getFullName();
    const IDetector *cDet = dynamic_cast<IDetector *>(cID);
    const detid_t cDetID = (cDet) ? cDet->getID() : 0;

    file << "	<component-link";
    if (cDetID != 0)
      file << " id=\"" << cDetID << "\"";
    file << " name=\"" << cFullName << "\">\n";
    for (const auto &param : comp.second) {
      const std::string pName = param.get<0>();
      const std::string pType = param.get<1>();
      const std::string pValue = param.get<2>();

      // With fitting parameters, we're actually inserting an entire element, as
      // constructed above
      if (pType == "fitting") {
        file << "		<parameter name=\"" << pName
             << "\" type=\"fitting\" >\n";
        file << "   " << pValue << "\n";
        file << "		</parameter>\n";
      } else {
        file << "		<parameter name=\"" << pName << "\""
             << (pType == "string" ? " type=\"string\"" : "") << ">\n";
        file << "			<value val=\"" << pValue << "\"/>\n";
        file << "		</parameter>\n";
      }
    }
    file << "	</component-link>\n";
  }
  file << "</parameter-file>\n";

  file.flush();
  file.close();
}

} // namespace DataHandling
} // namespace Mantid
