#ifndef MANTID_MDEVENTS_SLICINGALGORITHM_H_
#define MANTID_MDEVENTS_SLICINGALGORITHM_H_

    
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/SlicingAlgorithm.h"

namespace Mantid
{
namespace MDEvents
{

  /** Abstract Algorithm class that will be used by:
   *    BinMD and SliceMD
   * and shares code for getting a slice from one workspace to another
    
    @author Janik Zikovsky
    @date 2011-09-27

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport SlicingAlgorithm  : public API::Algorithm
  {
  public:
    SlicingAlgorithm();
    ~SlicingAlgorithm();
    
    ///@return a string with the character that identifies each dimension in order (XYZT)
    static std::string getDimensionChars()
    { return "XYZT"; }

  protected:
    
    /// Initialise the properties
    void initSlicingProps();

    void createTransform();

    void createGeneralTransform();
    void createAlignedTransform();

    void makeAlignedDimensionFromString(const std::string & str);
    void makeBasisVectorFromString(const std::string & str);

    Mantid::Geometry::MDImplicitFunction * getImplicitFunctionForChunk(size_t * chunkMin, size_t * chunkMax);
    Mantid::Geometry::MDImplicitFunction * getGeneralImplicitFunction(size_t * chunkMin, size_t * chunkMax);

    /// Input workspace
    Mantid::API::IMDWorkspace_sptr m_inWS;

    /// Original (MDEventWorkspace) that inWS was based on. Used during basis vector constructor
    Mantid::API::IMDWorkspace_sptr m_originalWS;

    /** Bin dimensions to actually use. These are NEW dimensions created,
     * or copied (not pointing to) the original workspace. */
    std::vector<Mantid::Geometry::MDHistoDimension_sptr> m_binDimensions;

    /// Index of the dimension in the MDEW for the dimension in the output. Only for axis-aligned slices
    std::vector<size_t> m_dimensionToBinFrom;

    /// Coordinate transformation to apply. This transformation
    /// contains the scaling that makes the output coordinate = bin indexes in the output MDHistoWorkspace.
    Mantid::API::CoordTransform * m_transform;

    /// Coordinate transformation to save in the output workspace (original->binned)
    Mantid::API::CoordTransform * m_transformFromOriginal;
    /// Coordinate transformation to save in the output workspace (binned->original)
    Mantid::API::CoordTransform * m_transformToOriginal;

    /// Set to true if the cut is aligned with the axes
    bool m_axisAligned;

    /// Number of dimensions in the output (binned) workspace.
    size_t m_outD;

    /// Basis vectors of the output dimensions, normalized to unity length
    std::vector<Mantid::Kernel::VMD> m_bases;

    /// Scaling factor to apply for each basis vector (to map to the bins)
    std::vector<double> m_scaling;

    /// Origin (this position in the input workspace = 0,0,0 in the output).
    Mantid::Kernel::VMD m_origin;


  };


} // namespace MDEvents
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_SLICINGALGORITHM_H_ */
