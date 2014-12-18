#ifndef MANTID_DATAHANDLING_SetSampleMaterial_H_
#define MANTID_DATAHANDLING_SetSampleMaterial_H_

//--------------------------------
// Includes
//--------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/NeutronAtom.h"

namespace Mantid
{
namespace DataHandling
{

/** 
    This class allows the shape of the sample to be defined by using the allowed XML
    expressions

    @author Vickie Lynch, SNS
    @date 2/7/2013

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport SetSampleMaterial : public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  SetSampleMaterial();
  /// Virtual destructor
  virtual ~SetSampleMaterial();
  /// Algorithm's name
  virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Sets the neutrons information in the sample.";}

  /// Algorithm's version
  virtual int version() const;
  /// Algorithm's category for identification
  virtual const std::string category() const;
  virtual std::map<std::string, std::string> validateInputs();

private:
  
  /// Initialisation code
  void init();
  ///Execution code
  void exec();
  /// Print out the list of information for the material
  void fixNeutron(PhysicalConstants::NeutronAtom &neutron,
                  double coh_xs, double inc_xs,
                  double abs_xs, double tot_xs);
};

}
}

#endif /* MANTID_DATAHANDLING_SetSampleMaterial_H_*/
