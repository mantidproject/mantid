//-------------------------------
// Includes
//-------------------------------
#include "MantidQtAPI/InterfaceFactory.h"

using namespace MantidQt::API;

//---------------------------------
// Private member functions
//--------------------------------
/**
 * Constructor
 */
InterfaceFactoryImpl::InterfaceFactoryImpl() : Mantid::Kernel::DynamicFactory<QWidget>()
{
}	

/**
 * Destructor
 */
InterfaceFactoryImpl::~InterfaceFactoryImpl()
{
}	

