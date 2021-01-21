// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {

/** DisjointElement : Cluster item used in a disjoint-set data structure.
 */
class MANTID_CRYSTAL_DLL DisjointElement {
public:
  /// Default constructor
  DisjointElement();
  /// Constructor
  DisjointElement(const int id);
  /// Destructor
  virtual ~DisjointElement() = default;
  /// Get Id
  int getId() const;
  /// Set the id
  void setId(int id);
  /// Get parent element
  DisjointElement *getParent() const;
  /// Get root id
  int getRoot() const;
  /// Union with other
  void unionWith(DisjointElement *other);
  /// Get the current rank
  int getRank() const;
  /// Increment the rank
  int incrementRank();
  /// Is empty.
  bool isEmpty() const;
  /// Copy constructor.
  DisjointElement(const DisjointElement &other);
  /// Assignment operator.
  DisjointElement &operator=(const DisjointElement &other);
  /// Less than
  inline bool operator<(const DisjointElement &other) const { return m_id < other.getId(); }
  /// Greater than
  inline bool operator>(const DisjointElement &other) const { return m_id > other.getId(); }

private:
  bool hasParent() const;
  int compress();
  void setParent(DisjointElement *other);

  // Data members
  /// Parent element
  DisjointElement *m_parent;
  /// Current rank
  int m_rank;
  /// Identifier
  int m_id;
};

void unionElements(DisjointElement *a, DisjointElement *b);

} // namespace Crystal
} // namespace Mantid
