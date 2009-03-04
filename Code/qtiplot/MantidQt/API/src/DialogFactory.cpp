//-------------------------------
// Includes
//-------------------------------
#include "MantidQtAPI/DialogFactory.h"

using namespace MantidQt::API;

//---------------------------------
// Private member functions
//--------------------------------
/**
 * Constructor
 */
DialogFactoryImpl::DialogFactoryImpl() : Mantid::Kernel::DynamicFactory<AlgorithmDialog>()
{
}	

/**
 * Destructor
 */
DialogFactoryImpl::~DialogFactoryImpl()
{
}	

