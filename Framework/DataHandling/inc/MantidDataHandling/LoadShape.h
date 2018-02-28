#ifndef DATAHANDLING_LOAD_SHAPE_H_
#define DATAHANDLING_LOAD_SHAPE_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace DataHandling {
/**  Load Shape into an instrument of a workspace

     The following file types are supported
 
       STL file with suffix .stl
 

@author Karl Palomen ISIS; initially extracted from Stuart Campbell's SaveNXSPE
algorithm,
@date 26/02/2018

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport LoadShape : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadShape"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The algorithm loads a shape into the instrument of a workspace "
           "at the specified place";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
  boost::shared_ptr<Geometry::MeshObject> readSTLSolid(std::ifstream &file, std::string &name);
  boost::shared_ptr<Geometry::MeshObject> readSTLMeshObject(std::ifstream &file);
  bool readSTLTriangle(std::ifstream &file, Kernel::V3D &v1, Kernel::V3D &v2, Kernel::V3D &v3);
  uint16_t addSTLVertex(Kernel::V3D &vertex, std::vector<Kernel::V3D> vertices);
  bool areEqualVertices(Kernel::V3D &v1, Kernel::V3D &v2) {
    Kernel::V3D diff = v1 - v2;
    return diff.norm() < 0.0000001;
  }
};



} // end namespace DataHandling
} // end namespace Mandid

#endif /* DATAHANDLING_LOAD_SHAPE_H_ */
