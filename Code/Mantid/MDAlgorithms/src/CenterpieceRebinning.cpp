#include "MantidMDAlgorithms/CenterpieceRebinning.h"

namespace Mantid{
    namespace MDAlgorithms{
        using namespace MDDataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CenterpieceRebinning)

CenterpieceRebinning::CenterpieceRebinning(void): API::Algorithm(), m_progress(NULL) 
{}

/** Destructor     */
CenterpieceRebinning::~CenterpieceRebinning()
{
    if( m_progress ){
	        delete m_progress;
    }
}
//
void
CenterpieceRebinning::init()
{
        declareProperty(new WorkspaceProperty<MDWorkspace>("InputWorkspace","",Direction::Input),
        "Name of the input workspace");
        declareProperty(new WorkspaceProperty<MDWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output workspace");
        BoundedValidator<double> *isDouble = new BoundedValidator<double>();
        declareProperty("Offset", 0.0, isDouble,
                "The amount to change each time bin by");
}
//
void 
CenterpieceRebinning::exec()
{
}


} //namespace MDAlgorithms
} //namespace Mantid