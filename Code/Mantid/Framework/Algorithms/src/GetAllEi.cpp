//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/GetAllEi.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(GetAllEi)

/// Empty default constructor
GetAllEi::GetAllEi() : Algorithm(), {}

/// Initialization method.
void GetAllEi::init() {

  declareProperty(
      new API::WorkspaceProperty<API::MatrixWorkspace>("Workspace", "",
                                                       Direction::Input),
      "The name of the input workspace to apply the calibration to");

  declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>(
                      "PositionTable", "", Direction::Input),
                  "The name of the table workspace containing the new "
                  "positions of detectors");
}

/** Executes the algorithm. Moving detectors of input workspace to positions
*indicated in table workspace
*
*  @throw FileError Thrown if unable to get instrument from workspace,
*                   table workspace is incompatible with instrument
*/
void GetAllEi::exec() {
  // Get pointers to the workspace, parameter map and table
  API::MatrixWorkspace_sptr inputWS = getProperty("Workspace");
  m_pmap = &(inputWS->instrumentParameters()); // Avoids a copy if you get the
                                               // reference before the
                                               // instrument

  API::ITableWorkspace_sptr PosTable = getProperty("PositionTable");
  Geometry::Instrument_const_sptr instrument = inputWS->getInstrument();
  if (!instrument) {
    throw std::runtime_error(
        "Workspace to apply calibration to has no defined instrument");
  }

  size_t numDetector = PosTable->rowCount();
  ColumnVector<int> detID = PosTable->getVector("Detector ID");
  ColumnVector<V3D> detPos = PosTable->getVector("Detector Position");
  // numDetector needs to be got as the number of rows in the table and the
  // detID got from the (i)th row of table.
  for (size_t i = 0; i < numDetector; ++i) {
    setDetectorPosition(instrument, detID[i], detPos[i], false);
  }
  // Ensure pointer is only valid for execution
  m_pmap = NULL;
}


} // namespace Algorithms
} // namespace Mantid
