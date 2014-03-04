#ifndef MANTID_CRYSTAL_CLUSTERITEM_H_
#define MANTID_CRYSTAL_CLUSTERITEM_H_

#include "MantidKernel/System.h"

namespace Mantid
{
namespace Crystal
{

  /** ClusterItem : Cluster item used in a disjoint-set data structure.
    
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
  class DLLExport ClusterItem 
  {
  public:
    ClusterItem(const int id);
    ClusterItem(const int id, ClusterItem * const parent);
    ClusterItem(const ClusterItem& other);
    ClusterItem& operator=(const ClusterItem& other);
    virtual ~ClusterItem();
    int getId() const;
    int getDepth() const; // TODO -review
    ClusterItem * getParent() const;
    int getRoot() const;
    int compress(); // TODO -review access
    bool hasParent() const; // TODO -review access
    void unionWith(ClusterItem& other);
    void setParent(ClusterItem * other); //TODO -review access
    void decrementRank(); // TODO - review access
    void incrementRank(); //TODO -review access
    int getRank() const;
  private:
    ClusterItem * m_parent;
    int m_depth;
    int m_rank;
    const int m_id;

    
  };




} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_CLUSTERITEM_H_ */
