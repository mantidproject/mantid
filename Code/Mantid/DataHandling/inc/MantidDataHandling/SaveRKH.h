#ifndef MANTID_DATAHANDLING_SAVERKH_H_
#define MANTID_DATAHANDLING_SAVERKH_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <fstream>

namespace Mantid
{
namespace DataHandling
{
/** Saves a workspace in the RKH file format

    Required properties:
    <UL>
    <LI> InputWorkspace - The name workspace to save.</LI>
    <LI> Filename - The path save the file</LI>
    <LI> Append - Whether to append to a file that already exists (true, the default), or overwrite</LI>
    </UL>

    @author Martyn Gigg, Tessella Support Services plc
    @date 26/01/2009
     
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
     
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
 */
class DLLExport SaveRKH : public API::Algorithm
{
public:
  /// Constructor
  SaveRKH();
  /// Virtual destructor
  virtual ~SaveRKH();
  /// Algorithm's name
  virtual const std::string name() const { return "SaveRKH"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "DataHandling"; }

  /// Constants used in RKH files
  enum FileConstants
  {
    Q_CODE = 6,                           ///< this is the integer code the RKH file format associates with the unit Q
    LINE_LENGTH = 8                       ///< the maximum number of numbers that a line can contain
  };

private:
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void writeHeader();
  void write1D();
  void write2D();

  /// The input workspace
  API::MatrixWorkspace_const_sptr m_workspace;
  /// Whether this is a 2D dataset
  bool m_2d;
  /// The output filehandle
  std::ofstream m_outRKH;
};

}
}

#endif //MANTID_DATAHANDLING_SAVERKH_H_
