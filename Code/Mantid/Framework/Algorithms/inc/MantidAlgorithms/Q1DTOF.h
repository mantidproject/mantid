#ifndef MANTID_ALGORITHMS_Q1D_H_
#define MANTID_ALGORITHMS_Q1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/GravitySANSHelper.h"

namespace Mantid
{
namespace Algorithms
{
/** Takes account of the effects of gravity for instruments where the y-axis points upwards, for
    example SANS instruments

    @author Steve Williams ISIS Rutherford Appleton Laboratory 
    @date 10/12/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport Q1DTOF : public API::Algorithm
{
public:
  /// (Empty) Constructor
  Q1DTOF() : API::Algorithm(), m_distr(true), m_numInBins(0) {}
  /// Virtual destructor
  virtual ~Q1DTOF() {}
  /// Algorithm's name
  virtual const std::string name() const { return "Q1DTOF"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// the experimental workspace with counts across the detector
  API::MatrixWorkspace_const_sptr m_dataWS;
  /// true if the input workspaces are distributions, the output will initially match the distribution status of the input and then change to a distribution
  bool m_distr;
  /// number of bin boundies in the input workspace
  size_t m_numInBins;

  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  void examineInput(API::MatrixWorkspace_const_sptr binAdj, API::MatrixWorkspace_const_sptr detectAdj);
  API::MatrixWorkspace_sptr setUpOutputWorkspace(const std::vector<double> & binParams,  const API::SpectraDetectorMap * const specMap) const;
  void convertWavetoQ(const size_t specIndex, const bool doGravity, MantidVec & Qs) const;
  void getNormFromSpec(API::MatrixWorkspace_const_sptr pixelAdj, const size_t specIndex, const MantidVec & binNorms, const MantidVec & binNormEs, MantidVec & outNorms, MantidVec & outEs) const;
  double pixelWeight(API::MatrixWorkspace_const_sptr pixelAdj,  const size_t specIndex) const;
  void getUnmaskedWidths(const size_t specIndex, MantidVec & widths) const;
  void updateSpecMap(const size_t specIndex, API::SpectraDetectorMap * const specMap, const Geometry::ISpectraDetectorMap & inSpecMap, API::MatrixWorkspace_sptr outputWS) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_Q1D_H_*/
