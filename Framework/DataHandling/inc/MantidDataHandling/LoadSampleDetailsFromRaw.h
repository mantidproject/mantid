#ifndef MANTIDDATAHANDLING_LOADSAMPLEDETAILSFROMRAW_H_
#define MANTIDDATAHANDLING_LOADSAMPLEDETAILSFROMRAW_H_

//-----------------------------------------------------
// Includes
//-----------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/**
   An algorithm to extract the sample details from the SPB structure within a
   RAW file

   Required properties:
   <UL>
   <LI>InputWorkspace - The workspace to add information to</LI>
   <LI>Filename - The raw file to use to gather the information</LI>
   </UL>

   @author Martyn, Tessella plc
   @date 29/07/2009

   Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

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

   File change history is stored at: <https://github.com/mantidproject/mantid>.
   Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport LoadSampleDetailsFromRaw : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadSampleDetailsFromRaw"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads the simple sample geometry that is defined within an ISIS "
           "raw file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadRaw"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Raw;Sample";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
}
}

#endif /*MANTIDDATAHANDLING_LOADSAMPLEDETAILSFROMRAW_H_*/
