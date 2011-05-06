#include "MDDataObjects/MDWorkspaceIterator.h"
namespace Mantid
{
  namespace MDDataObjects
  {

    MDWorkspaceIterator::MDWorkspaceIterator(const MDWorkspaceIndexCalculator& indexCalculator,

      const std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions):

    m_indexCalculator(indexCalculator),m_cur_pointer(),m_end_pointer(indexCalculator.getIndexUpperBounds()), m_dimensions(dimensions)
    {
      m_index.resize(indexCalculator.getNDimensions());
      indexCalculator.calculateDimensionIndexes(m_cur_pointer,m_index);
    }

    MDWorkspaceIterator::~MDWorkspaceIterator()
    {

    }

    size_t MDWorkspaceIterator::getDataSize()const
    {
      return m_indexCalculator.getIndexUpperBounds() + 1;
    }

    double MDWorkspaceIterator::getCoordinate(int i)const
    {
      return m_dimensions[i]->getX(m_index[i]);
    }

    bool MDWorkspaceIterator::next()
    {
      if (m_cur_pointer < m_end_pointer)
      {
        ++m_cur_pointer;
        m_indexCalculator.calculateDimensionIndexes(m_cur_pointer,m_index);
        return true;
      }
      return false;
    }

    size_t MDWorkspaceIterator::getPointer()const
    {
      return m_cur_pointer;
    }

  }
}