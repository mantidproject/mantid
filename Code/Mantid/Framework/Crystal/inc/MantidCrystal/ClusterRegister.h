#ifndef MANTID_CRYSTAL_CLUSTERREGISTER_H_
#define MANTID_CRYSTAL_CLUSTERREGISTER_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "MantidCrystal/DisjointElement.h"
#include <map>
#include <vector>

namespace Mantid
{
namespace Crystal
{
  class ICluster;
  class ImplClusterRegister;

  /** ClusterRegister : A fly-weight ICluster regeister. Handles the logic of merging clusters.
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport ClusterRegister 
  {
  public:

    typedef std::map<size_t, boost::shared_ptr<ICluster> >  MapCluster;

    ClusterRegister();

    void add(const size_t& label, const boost::shared_ptr<ICluster>& cluster);

    void merge(const DisjointElement& a, const DisjointElement& b) const;

    void toUniformMinimum(std::vector<DisjointElement>& elements);

    MapCluster clusters() const;

    virtual ~ClusterRegister();
    
  private:

    boost::scoped_ptr<ImplClusterRegister> m_Impl;

  };


} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_CLUSTERREGISTER_H_ */
