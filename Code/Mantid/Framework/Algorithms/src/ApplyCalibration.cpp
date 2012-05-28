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
      // Get pointers to the workspace and table
      API::MatrixWorkspace_sptr Ws = getProperty("Workspace");
      API::ITableWorkspace_sptr PosTable = getProperty("PositionTable");

      // Have to resort to a cast here as in UpdayeInstrumentFromFile.cpp
      // Actually we are changing position via the instrument parameters, so may be OK with const instrument here
      Instrument_sptr instrument = boost::const_pointer_cast<Instrument>(Ws->getInstrument()->baseInstrument());
      if (instrument.get() == 0)
      {
        throw std::runtime_error("Workspace to apply calibration to has no defined instrument");
      }

      int numDetector = PosTable->rowCount();
      //API::Column_const_sptr detID = PosTable->getColumn( 0 );
      ColumnVector<int> detID = PosTable->getVector("Detector ID");
      ColumnVector<V3D> detPos = PosTable->getVector("Detector Position");
      // numDetector needs to be got as the number of rows in the table and the detID got from the (i)th row of table.
      for (int i = 0; i < numDetector; ++i)
      {
        Geometry::IDetector_sptr det = boost::const_pointer_cast<Geometry::IDetector>(instrument->getDetector(detID[i]));
        setDetectorPosition( Ws, instrument, detID[i], detPos[i], false );
      }

    }

    /**
    * Set the absolute detector position of a detector
    * @param Ws: The workspace containing detectors to be moved
    * @param instrument :: A shared pointer to the base instrument
    * @param detID :: Detector ID
    * @param pos :: new position of Dectector
    * @param sameParent :: true if detector has same parent as previous detector set here.
    */
    void ApplyCalibration::setDetectorPosition(API::MatrixWorkspace_sptr Ws, boost::shared_ptr<Geometry::Instrument> instrument, int detID, V3D pos, bool sameParent )
    {
       Geometry::IDetector_sptr det = boost::const_pointer_cast<Geometry::IDetector>(instrument->getDetector(detID)); 

       if (det == 0)
       {
         std::ostringstream mess;
         mess<<"Detector with ID "<<detID<<" was not found.";
         g_log.error(mess.str());
         throw std::runtime_error(mess.str());
       }

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

       Geometry::ParameterMap& pmap = Ws->instrumentParameters();
       // Add a parameter for the new position
       pmap.addV3D(det.get(), "pos", pos);


    }

  } // namespace Algorithms
} // namespace Mantid
