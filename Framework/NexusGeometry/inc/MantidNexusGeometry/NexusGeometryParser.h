//
// Created by michael on 23/08/17.
//

#ifndef MANTID_NEXUS_GEOMETRY_PARSER_H_
#define MANTID_NEXUS_GEOMETRY_PARSER_H_

//---------------------------------
// Includes
//---------------------------------

#include "MantidNexusGeometry/ParsingErrors.h"
// All possible derived classes from InstrumentAbstractBuilder
#include "MantidNexusGeometry/InstrumentGeometryAbstraction.h"

#include <Eigen/Core> 
#include <Eigen/Geometry>
#include <H5Cpp.h>

#include <memory>
#include <vector>

namespace Mantid {
namespace NexusGeometry {

//Choose which derived instrumentAbstraction to use
typedef std::weak_ptr<InstrumentGeometryAbstraction> iAbstractBuilder_wkpr;

class NexusGeometryParser
{
public:
    /// Constructor // , std::weak_ptr<InstrumentHandler> iHandler
    explicit NexusGeometryParser(const H5std_string &fileName);
    
    /// Destructor
    ~NexusGeometryParser() = default;
    
    /// OFF INSTRUMENT GEOMETRY PARSER - returns exit status to LoadNexusGeometry
    ParsingErrors ParseNexusGeometry();
    
private:
    H5::H5File nexusFile;
    H5::Group rootGroup;
    ParsingErrors exitStatus = NO_ERROR;
    ///Instrument abstraction builder
    iAbstractBuilder_wkpr iBuilder_wkpr;
    /// Opens sub groups of current group
    std::vector<H5::Group> openSubGroups(H5::Group &parentGroup, H5std_string CLASS_TYPE);
    /// Opens all detector groups in a file
    std::vector<H5::Group> openDetectorGroups ();
    /// Stores detectorGroup pixel offsets as Eigen 3xN matrix
    Eigen::Matrix<double, 3, Eigen::Dynamic> getPixelOffsets(H5::Group &detectorGroup);
    ///Gets the transformations applied to the detector's pixelOffsets
    Eigen::Transform<double, 3, Eigen::Affine> getTransformations(H5::Group &detectorGroup);
    ///Gets the data from a string dataset
    H5std_string get1DStringDataset(H5std_string &dataset);
    ///Read dataset into vector
    template<typename valueType> std::vector<valueType> get1DDataset(H5std_string &dataset);
};

}
}

#endif // MANTID_NEXUS_GEOMETRY_PARSER_H_
