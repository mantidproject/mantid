#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"

namespace Mantid
{
  namespace API
  {
      //-----------------------------------------------------------------------------------------------
      /** Default constructor */
      IMDWorkspace::IMDWorkspace()
      : Workspace(),
        Mantid::API::MDGeometry()
      {
      }

      //-----------------------------------------------------------------------------------------------
      /** Copy constructor */
      IMDWorkspace::IMDWorkspace(const IMDWorkspace & other)
      : Workspace(other),
        Mantid::API::MDGeometry(other)
      {
      }

      /// Destructor
      IMDWorkspace::~IMDWorkspace()
      {
      }

      /** Creates a single iterator and returns it.
       *
       * This calls createIterators(), a pure virtual method on IMDWorkspace which
       * has custom implementations for other workspaces.
       *
       * @param function :: Implicit function limiting space to look at
       * @return a single IMDIterator pointer
       */
      IMDIterator* IMDWorkspace::createIterator(Mantid::Geometry::MDImplicitFunction * function) const
      {
        std::vector<IMDIterator*> iterators = this->createIterators(1, function);
        if (iterators.empty())
          throw std::runtime_error("IMDWorkspace::createIterator(): iterator creation was not successful. No iterators returned by " + this->id() );
        return iterators[0];
      }


      //-------------------------------------------------------------------------------------------
      /** Returns the signal (normalized by volume) at a given coordinates
       *
       * @param coords :: coordinate as a VMD vector
       * @param normalization :: how to normalize the signal returned
       * @return normalized signal
       */
      signal_t IMDWorkspace::getSignalAtVMD(const Mantid::Kernel::VMD & coords,
          const Mantid::API::MDNormalization & normalization) const
      {
        return this->getSignalAtCoord(coords.getBareArray(), normalization);
      }

  }
}

namespace Mantid
{
namespace Kernel
{
  /** In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IMDEventWorkspace> is required */
  template<> MANTID_API_DLL
  Mantid::API::IMDWorkspace_sptr IPropertyManager::getValue<Mantid::API::IMDWorkspace_sptr>(const std::string &name) const
  {
    PropertyWithValue<Mantid::API::IMDWorkspace_sptr>* prop =
                      dynamic_cast<PropertyWithValue<Mantid::API::IMDWorkspace_sptr>*>(getPointerToProperty(name));
    if (prop)
    {
      return *prop;
    }
    else
    {
      std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected IMDWorkspace.";
      throw std::runtime_error(message);
    }
  }

  /** In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IMDWorkspace_const_sptr> is required */
  template<> MANTID_API_DLL
  Mantid::API::IMDWorkspace_const_sptr IPropertyManager::getValue<Mantid::API::IMDWorkspace_const_sptr>(const std::string &name) const
  {
    PropertyWithValue<Mantid::API::IMDWorkspace_sptr>* prop =
                      dynamic_cast<PropertyWithValue<Mantid::API::IMDWorkspace_sptr>*>(getPointerToProperty(name));
    if (prop)
    {
      return prop->operator()();
    }
    else
    {
      std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const IMDWorkspace.";
      throw std::runtime_error(message);
    }
  }

} // namespace Kernel
} // namespace Mantid

