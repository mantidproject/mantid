#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/ImplicitTopology.h"
#include "MantidAPI/Point3D.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
         void ImplicitTopology::applyOrdering(Mantid::API::Point3D** unOrderedPoints) const
         {
             //For an implicit topology, i.e. a structured grid, Do nothing.
         }

         std::string ImplicitTopology::getName() const
         {
             return "ImplicitTopology";
         }

         std::string ImplicitTopology::toXMLString() const
         {
             return boost::str(boost::format("<Topology><Type>%s</Type></Topology>") % getName());
         }
    }


}