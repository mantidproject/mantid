#ifndef MANTID_ALGORITHMS_CORRECTFORATTENUATION_H_
#define MANTID_ALGORITHMS_CORRECTFORATTENUATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Object.h"

namespace Mantid
{
namespace Algorithms
{
/** Calculates bin-by-bin correction factors for attenuation due to absorption and
    scattering in a cylindrical sample.

    @author Russell Taylor, Tessella Support Services plc
    @date 02/12/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport CorrectForAttenuation : public API::Algorithm
{
public:
  /// (Empty) Constructor
  CorrectForAttenuation() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~CorrectForAttenuation() {}
  /// Algorithm's name
  virtual const std::string name() const { return "CorrectForAttenuation"; }
  /// Algorithm's version
  virtual const int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "General"; }

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void retrieveProperties();
  void constructCylinderSample();
  void calculateDistances(const Geometry::V3D& detectorPos);
  double doIntegration(const double& lambda);

  /// Sample object. Keeping separate from the full instrument geometry for now.
  Geometry::Object m_cylinderSample;
  double m_cylHeight;   ///< The height of the cylindrical sample
  double m_cylRadius;   ///< The radius of the cylindrical sample
  double m_refAtten;    ///< The attenuation factor at 1.8A
  double m_scattering;  ///< The scattering factor
  std::vector<double> m_L1s,  ///< Cached distances
                      m_L2s,  ///< Cached distances
                      m_elementVolumes;  ///< Cached element volumes

  /// Static reference to the logger class
  static Kernel::Logger& g_log;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CORRECTFORATTENUATION_H_*/
