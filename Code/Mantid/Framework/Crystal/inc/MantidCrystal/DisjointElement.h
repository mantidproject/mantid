#ifndef MANTID_CRYSTAL_DISJOINTELEMENT_H_
#define MANTID_CRYSTAL_DISJOINTELEMENT_H_

#include "MantidKernel/System.h"

namespace Mantid
{
namespace Crystal
{

  /** DisjointElement : Cluster item used in a disjoint-set data structure.
    
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
  class DLLExport DisjointElement
  {
  public:
    /// Default constructor
    DisjointElement();
    /// Constructor
    DisjointElement(const int id);
    /// Destructor
    virtual ~DisjointElement();
    /// Get Id
    int getId() const;
    /// Set the id
    void setId(int id);
    /// Get parent element
    DisjointElement * getParent() const;
    /// Get root id
    int getRoot() const;
    /// Union with other
    void unionWith(DisjointElement* other);
    /// Get the current rank
    int getRank() const;
    /// Increment the rank
    int incrementRank();
    /// Is empty.
    bool isEmpty() const;
    /// Copy constructor.
    DisjointElement(const DisjointElement& other);
    /// Assignment operator.
    DisjointElement& operator=(const DisjointElement& other);
    /// Less than
    inline bool operator<(const DisjointElement& other) const 
    {
      return m_id < other.getId();
		}
    /// Greater than
    inline bool operator>(const DisjointElement& other) const 
    {
      return m_id > other.getId();
    }
		
  private:
    bool hasParent() const;
    int compress();
    void setParent(DisjointElement * other);

    // Data members
    /// Parent element
    DisjointElement * m_parent;
    /// Current rank
    int m_rank;
    /// Identifier
    int m_id;
    
  };

  void unionElements(DisjointElement* a, DisjointElement* b);

} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_DISJOINTELEMENT_H_ */
