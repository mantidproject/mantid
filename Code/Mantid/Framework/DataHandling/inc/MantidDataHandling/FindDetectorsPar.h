#ifndef DATAHANDLING_FIND_DETPAR_H_
#define DATAHANDLING_FIND_DETPAR_H_
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

/** An algorithm to calculate workspace detectors angular coordinates, as they can be viewed from a sample (par or phx data)

    Required Properties:
    <UL>
    <LI> Workspace - The name of the input Workspace2D on which to perform the algorithm </LI>
    </UL>

    Output Properties:

	<UL><LI> azimuthal            - An array property containing the detectors azimutal angles</LI> </UL>
    <UL><LI> polar                - An array property containing the detectors polar angles</LI>    </UL>
    <UL><LI> azimuthal_width      - An array property containing the detectors azimuthal angular width</LI></UL>
    <UL><LI> polar_width          - An array property containing the detectors polar angular width</LI></UL>
    <UL><LI> secondary_flightpath - An array property containing the distance from detectors to the sample center</LI></UL>



    @author Alex Buts ISIS; initially extracted from Stuart Campbell's SaveNXSPE algorithm,
    @date 17/05/2012

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace Mantid
{
namespace DataHandling
{
class DLLExport FindDetectorsPar : public API::Algorithm
{
public:
  FindDetectorsPar();
  virtual ~FindDetectorsPar();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "FindDetectorsPar";};
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;};
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling";}
  /// the accessors, used to return algorithm results when called as sub-algorithm, without setting the properties;
  std::vector<double>const & getAzimuthal()const{return azimuthal;}
  std::vector<double>const & getPolar()const{return polar;}
  std::vector<double>const & getAzimWidth()const{return azimuthal_width;}
  std::vector<double>const & getPolarWidth()const{return polar_width;}
  std::vector<double>const & getFlightPath()const{return secondary_flightpath;}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Implement abstract Algorithm methods
  void init();
  void exec();

  std::vector<double> azimuthal;
  std::vector<double> polar;
  std::vector<double> azimuthal_width;
  std::vector<double> polar_width;
  std::vector<double> secondary_flightpath;
  /// auxiliary function, which transforms array into a string
  void fill_property(Kernel::Property *const,std::vector<double> const &);
};


} //end namespace DataHandling
} //end namespace Mandid


#endif
