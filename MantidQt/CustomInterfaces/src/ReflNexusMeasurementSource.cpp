#include "MantidQtCustomInterfaces/ReflNexusMeasurementSource.h"
#include <Poco/File.h>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <iostream>
#include <string>
#include <sstream>



using namespace Mantid::API;

namespace MantidQt{
namespace CustomInterfaces {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ReflNexusMeasurementSource::ReflNexusMeasurementSource() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReflNexusMeasurementSource::~ReflNexusMeasurementSource() {}

Measurement ReflNexusMeasurementSource::obtain(const std::string &definedPath, const std::string &fuzzyName) const
{
    std::string filenameArg = fuzzyName;
    if(!definedPath.empty()) {
    Poco::File file(definedPath);
    if(file.exists() && file.isFile()){
        // Load the exact path
        filenameArg = definedPath;
    }}
    try{

    const std::string filenameArg = fuzzyName;
    IAlgorithm_sptr algLoadRun = AlgorithmManager::Instance().create("LoadISISNexus");
    algLoadRun->setChild(true);
    algLoadRun->setRethrows(true);
    algLoadRun->initialize();
    algLoadRun->setProperty("Filename", filenameArg);
    algLoadRun->setPropertyValue("OutputWorkspace", "dummy");
    algLoadRun->execute();
    Workspace_sptr temp = algLoadRun->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);


    return Measurement::InvalidMeasurement("Not yet implemented");

    }
    catch(std::runtime_error& ex){
        std::stringstream buffer;
        buffer << "Meta-data load attemped a load using: " << filenameArg << std::endl;
        buffer << ex.what();
        const std::string message = buffer.str();
        return Measurement::InvalidMeasurement(message);
    }



}

ReflNexusMeasurementSource *ReflNexusMeasurementSource::clone() const
{
    return new ReflNexusMeasurementSource(*this);
}


} // namespace CustomInterfaces
} // namespace MantidQt
