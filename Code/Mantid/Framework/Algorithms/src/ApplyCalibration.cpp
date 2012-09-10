/*WIKI* 

To be written


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ApplyCalibration.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidNexusCPP/NeXusException.hpp"
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
  namespace Algorithms
  {

    DECLARE_ALGORITHM(ApplyCalibration)

    /// Sets documentation strings for this algorithm
    void ApplyCalibration::initDocs()
    {
      this->setWikiSummary(" TBA ");
      this->setOptionalMessage(" TBA ");
    }
    
    using namespace Kernel;
    using namespace API;
    using Geometry::Instrument;
    using Geometry::Instrument_sptr;
    using Geometry::IDetector_sptr;
    using Kernel::V3D;

    /// Empty default constructor
    ApplyCalibration::ApplyCalibration()
      : Algorithm(), m_pmap(NULL)
    {}

    /// Initialisation method.
    void ApplyCalibration::init()
    {

      declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("Workspace", "",
        Direction::InOut), "The name of the input workspace to apply the calibration to");

      declareProperty(new API::WorkspaceProperty<API::ITableWorkspace>("PositionTable", "",
        Direction::Input), "The name of the table workspace containing the new positions of detectors");

    }

    /** Executes the algorithm. Moving detectors of input workspace to positions indicated in table workspace
    *
    *  @throw FileError Thrown if unable to get instrument from workspace, 
    *                   table workspace is incompatible with instrument
    */
    void ApplyCalibration::exec()
    {
      // Get pointers to the workspace, parameter map and table
      API::MatrixWorkspace_sptr inputWS = getProperty("Workspace");
      m_pmap = &(inputWS->instrumentParameters()); // Avoids a copy if you get the reference before the instrument

      API::ITableWorkspace_sptr PosTable = getProperty("PositionTable");
      Geometry::Instrument_const_sptr instrument = inputWS->getInstrument();
      if(!instrument)
      {
        throw std::runtime_error("Workspace to apply calibration to has no defined instrument");
      }

      size_t numDetector = PosTable->rowCount();
      ColumnVector<int> detID = PosTable->getVector("Detector ID");
      ColumnVector<V3D> detPos = PosTable->getVector("Detector Position");
      // numDetector needs to be got as the number of rows in the table and the detID got from the (i)th row of table.
      for (size_t i = 0; i < numDetector; ++i)
      {
        setDetectorPosition(instrument, detID[i], detPos[i], false );
      }
      // Ensure pointer is only valid for execution
      m_pmap = NULL;
    }

    /**
    * Set the absolute detector position of a detector
    * @param instrument :: The instrument that contains the defined detector
    * @param detID :: Detector ID
    * @param pos :: new position of Dectector
    * @param sameParent :: true if detector has same parent as previous detector set here.
    */
    void ApplyCalibration::setDetectorPosition(const Geometry::Instrument_const_sptr & instrument, int detID, V3D pos, bool /*sameParent*/ )
    {
       Geometry::IDetector_const_sptr det = instrument->getDetector(detID);
       // Then find the corresponding relative position
       boost::shared_ptr<const Geometry::IComponent> parent = det->getParent();
       if (parent)
       {
         pos -= parent->getPos();
         Quat rot = parent->getRelativeRot();
         rot.inverse();
         rot.rotate(pos);
       }
       boost::shared_ptr<const Geometry::IComponent>grandparent = parent->getParent();
       if (grandparent)
       {
         Quat rot = grandparent->getRelativeRot();
         rot.inverse();
         rot.rotate(pos);
       }

       // Add a parameter for the new position
       m_pmap->addV3D(det.get(), "pos", pos);
    }

  } // namespace Algorithms
} // namespace Mantid
