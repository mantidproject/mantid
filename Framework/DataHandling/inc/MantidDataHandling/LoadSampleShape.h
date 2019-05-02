// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DATAHANDLING_LOAD_SHAPE_H_
#define DATAHANDLING_LOAD_SHAPE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/LoadShape.h"
#include "MantidGeometry/Objects/MeshObject.h"

namespace Mantid {
namespace DataHandling {
/**  Load Shape into an instrument of a workspace

     The following file types are supported

       STL file with suffix .stl


@author Karl Palmen ISIS;
@date 26/02/2018
 */

class DLLExport LoadSampleShape : public Mantid::API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadSampleShape"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The algorithm loads a shape into the instrument of a workspace "
           "at the sample.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  /// Related algorithms
  const std::vector<std::string> seeAlso() const override {
    return {"CreateSampleShape", "CopySample", "SetSampleMaterial",
            "LoadSampleEnvironment"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Instrument";
  }
  boost::shared_ptr<Geometry::MeshObject>
  rotate(boost::shared_ptr<Geometry::MeshObject> sampleMesh,
         API::MatrixWorkspace_const_sptr inputWS);

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
  boost::shared_ptr<Geometry::MeshObject> loadStl(std::string filename,
                                        ScaleUnits scaleType);
};

} // namespace DataHandling
} // namespace Mantid
#endif /* DATAHANDLING_LOAD_SHAPE_H_ */
