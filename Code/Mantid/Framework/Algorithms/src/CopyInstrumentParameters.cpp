/*WIKI* 

Transfer an instrument from a giving workspace to a receiving workspace for the same instrument.

The instrument in of the receiving workspace is replaced by a copy of the instrument in the giving workspace
and so gains any manipulations such as calibration done to the instrument in the giving workspace.
The two workspaces can have spectra allocated to their detectors differently.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CopyInstrumentParameters.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <iostream>
#include "MantidAPI/MemoryManager.h"
#include "MantidGeometry/Instrument/ParameterMap.h"

namespace Mantid
{
namespace Algorithms
{

using std::size_t;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CopyInstrumentParameters)

/// Sets documentation strings for this algorithm
void CopyInstrumentParameters::initDocs()
{
  this->setWikiSummary("Transfers an instrument from on workspace to another workspace with same base instrument.");
  this->setOptionalMessage("Transfers an instrument from on workspace to another workspace with same base instrument.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
///using namespace DataObjects;

/// Default constructor
CopyInstrumentParameters::CopyInstrumentParameters() : 
  Algorithm(),
  m_different_instrument_sp(false)
{}

/// Destructor
CopyInstrumentParameters::~CopyInstrumentParameters() {}

void CopyInstrumentParameters::init()
{
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
    "Name of the workspace giving the instrument" );
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::InOut),
    "Name of the workspace receiving the instrument" );
}

/** Executes the algorithm
 *  @throw std::out_of_range If a property is set to an invalid value for the input workspace
 */
void CopyInstrumentParameters::exec()
{

  // Get the giving workspace
  m_givingWorkspace = getProperty("InputWorkspace");

  // Get the receiving workspace
  m_receivingWorkspace = getProperty("OutputWorkspace"); 

  // Retrieve and validate the input properties
  this->checkProperties();

  // Get parameters
  Geometry::ParameterMap& givParams = m_givingWorkspace->instrumentParameters() ;

  if (m_different_instrument_sp)
  {
    Instrument_const_sptr inst1 = m_givingWorkspace->getInstrument();
    Instrument_const_sptr inst2 = m_receivingWorkspace->getInstrument();
    auto Name1=inst1->getName();
    auto Name2=inst2->getName();

    Geometry::ParameterMap targMap;

    //// vector of all components contained in the target instrument
    //std::vector<IComponent_const_sptr> targComponents;
    //// flattens instrument definition tree
    //inst2->getChildren(targComponents,true);
    //// multimap of existing instrument parameters
    //std::multimap<std::string,IComponent const *> existingComponents;
    //for(size_t i=0;i<targComponents.size();i++)
    //{        
    //   if (dynamic_cast<IDetector const *>(targComponents[i].get()))
    //     continue;
    //   existingComponents.insert(std::pair<std::string,IComponent const *>(targComponents[i]->getFullName(),targComponents[i].get()));
    //}

    auto it = givParams.begin();
    for(;it!= givParams.end(); it++)
    {
      IComponent * oldComponent=it->first;


      const Geometry::IComponent* targComp = 0;

      IDetector *pOldDet = dynamic_cast<IDetector *>(oldComponent);
      if (pOldDet)
      {
        detid_t detID = pOldDet->getID();
        targComp = inst2->getBaseDetector(detID);
        if (!targComp)
        {
          g_log.warning()<<"Target instrument does not have detector with ID "<<detID<<'\n';
          continue;
        }
      }
      else
      {
        std::string source_name=oldComponent->getFullName();
        size_t nameStart = source_name.find(Name1);
        std::string targ_name = source_name.replace(nameStart,nameStart+Name1.size(),Name2);
        //existingComponents.
        auto spTargComp = inst2->getComponentByName(targ_name);
        if (!spTargComp)
        {
          g_log.warning()<<"Target instrument does not have component with full name: "<<targ_name<<'\n';
          continue;
        }
        targComp = spTargComp->getBaseComponent();
      }
      // merge maps for existing target component
      auto param = it->second.get();
      targMap.add(param->type(),targComp,param->name(),param->asString());

    }

    // changed parameters
    m_receivingWorkspace->replaceInstrumentParameters( targMap );

  }
  else
  {
    // unchanged Copy parameters
    m_receivingWorkspace->replaceInstrumentParameters( givParams );

  }


}


/** Retrieves the properties and checks that they have valid values.
 *  @throw std::invalid_argument If either workspace has no instrument or the instruments have different base instruments.
 */
void CopyInstrumentParameters::checkProperties()
{

  // Check that both workspaces have an instrument
  Instrument_const_sptr inst = m_givingWorkspace->getInstrument();
  if( !inst )
  {
      throw std::invalid_argument("Input workspace has no instrument");
  }
  Instrument_const_sptr inst2 = m_receivingWorkspace->getInstrument();
  if( !inst2 )
  {
      throw std::invalid_argument("Output workspace has no instrument");
  }

  Instrument_const_sptr baseInstGiv = inst->baseInstrument();
  Instrument_const_sptr baseInstRec = inst2->baseInstrument();

  // Check that both workspaces have the same instrument name
  if( baseInstRec != baseInstGiv )
  {
    m_different_instrument_sp=true;
    g_log.warning() << "The base instrument in the output workspace is not the same as the base instrument in the input workspace."<< std::endl;
  }

}

} // namespace Algorithms
} // namespace Mantid
