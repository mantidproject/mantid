#ifndef MANTID_ALGORITHM_FFT_H_
#define MANTID_ALGORITHM_FFT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Workspace.h"

namespace Mantid
{
namespace Algorithms
{

/**  A dummy unit which purpose is to have a name. It cannot be converted to or from TOF.
 */
class LabelUnit: public Kernel::Unit
{
    /// Caption
    const std::string m_caption;
    /// Label
    const std::string m_label;
public:
  /// Constructor
    LabelUnit():m_caption("Quantity"),m_label("units"){}
  /**  Constructor
   *   @param capt The unit's caption
   *   @param lbl  The unit's label
   */
    LabelUnit(const std::string& capt, const std::string& lbl):m_caption(capt),m_label(lbl){}
  /// The name of the unit. For a concrete unit, this method's definition is in the DECLARE_UNIT
  /// macro and it will return the argument passed to that macro (which is the unit's key in the
  /// factory).
    const std::string unitID() const {return "Label";};
  /// The full name of the unit
    const std::string caption() const {return m_caption;};
  /// A label for the unit to be printed on axes
    const std::string label() const {return m_label;};
  /** Convert from the concrete unit to time-of-flight. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void toTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const 
  {throw Kernel::Exception::NotImplementedError("Cannot convert this unit to time of flight");};

  /** Convert from time-of-flight to the concrete unit. TOF is in microseconds.
   *  @param xdata    The array of X data to be converted
   *  @param ydata    Not currently used (ConvertUnits passes an empty vector)
   *  @param l1       The source-sample distance (in metres)
   *  @param l2       The sample-detector distance (in metres)
   *  @param twoTheta The scattering angle (in radians)
   *  @param emode    The energy mode (0=elastic, 1=direct geometry, 2=indirect geometry)
   *  @param efixed   Value of fixed energy: EI (emode=1) or EF (emode=2) (in meV)
   *  @param delta    Not currently used
   */
  void fromTOF(std::vector<double>& xdata, std::vector<double>& ydata, const double& l1, const double& l2,
      const double& twoTheta, const int& emode, const double& efixed, const double& delta) const
  {throw Kernel::Exception::NotImplementedError("Cannot convert time of flight to this unit ");};

};

/** Performs a Fast Fourier Transform of data

    @author Roman Tolchenov
    @date 07/07/2009

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
class DLLExport FFT : public API::Algorithm
{
public:
  /// Default constructor
  FFT() : API::Algorithm() {};
  /// Destructor
  virtual ~FFT() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FFT";}
  /// Algorithm's version for identification overriding a virtual method
  virtual const int version() const { return 1;}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "General";}

private:
  // Overridden Algorithm methods
  void init();
  void exec();

};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_FFT_H_*/
