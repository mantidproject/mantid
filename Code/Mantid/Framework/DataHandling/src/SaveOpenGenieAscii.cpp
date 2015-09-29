//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveOpenGenieAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <exception>

#include <vector>

#include <list>

namespace Mantid {
    namespace DatHandling {
        
        using namespace Kernel;
        using namespace Mantid::API;
        
        // Register the algorithm into the AlgorithmFactory
        DECLARE_ALGORITHM(SaveOpenGenieAscii)
        
        /// Empty constructor
        SaveOpenGenieAscii::SaveOpenGenieAscii() : Mantid::API::Algorithm() {}
        
        //---------------------------------------------------
        // Private member functions
        //---------------------------------------------------
        /**
         * Initialise the algorithm
         */
        
        void SaveOpenGenieAscii::init() {
            declareProperty(
                            new API::WorkspaceProperty<>("InputWorkspace", "",
                                                         Kernel::Direction::Input),
                            "The name of the workspace containing the data you wish to save");
            
            // Declare required parameters, filename with ext {.his} and input
            // workspac
            std::vector<std::string> his_exts;
            his_exts.push_back(".his");
            his_exts.push_back(".txt");
            his_exts.push_back("");
            declareProperty(
                            new API::FileProperty("Filename", "", API::FileProperty::Save, his_exts),
                            "The filename to use for the saved data");
            declareProperty("IncludeHeader", true,
                            "Whether to include the header lines (default: true)");
        }
        
        void SaveOpenGenieAscii::exec() {
            // Process properties
            
            // Retrieve the input workspace
            /// Workspace
            ws = getProperty("InputWorkspace");
            int nSpectra = static_cast<int>(ws->getNumberHistograms());
            int nBins = static_cast<int>(ws->blocksize());
            
            // Retrieve the filename from the properties
            std::string filename = getProperty("Filename");
            
            // Output string variables
            const std::string singleSpc = " ";
            const std::string fourspc = "    ";
            
            // file
            std::ofstream outfile(filename.c_str());
            if (!outfile) {
                g_log.error("Unable to create file: " + filename);
                throw Exception::FileError("Unable to create file: ", filename);
            }
            if (nBins == 0 || nSpectra == 0)
                throw std::runtime_error("Trying to save an empty workspace");
            
            // Axis alphabets
            const std::string Alpha[] = {"\"e\"", "\"x\"", "\"y\""};
            
            const bool headers = getProperty("IncludeHeader");
            // if true write file header
            if (headers) {
                writeFileHeader(outfile);
            }
            
            bool isHistogram = ws->isHistogramData();
            Progress progress(this, 0, 1, nBins);
            std::string alpha;
            for (int Num = 0; Num < 3; Num++) {
                alpha = Alpha[Num];
                writeToFile(alpha, outfile, singleSpc, fourspc, nBins, isHistogram);
            }
            
            // outfile << std::endl;
            
            writeSampleLogs(outfile, fourspc);
            
            progress.report();
            return;
        }
        
        // -----------------------------------------------------------------------------
        /** generates the OpenGenie file header
         *  @param outfile :: File it will save it out to
         */
        void SaveOpenGenieAscii::writeFileHeader(std::ofstream &outfile) {
            
            const std::vector<Property *> &logData = ws->run().getLogData();
            auto &log = logData;
            // get total number of sample logs
            auto samplenumber = (&log)->size();
            samplenumber += 3; // x, y, e
            
            outfile << "# Open Genie ASCII File #" << std::endl
            << "# label " << std::endl
            << "GXWorkspace" << std::endl
            // number of entries
            << samplenumber << std::endl;
        }
        
        //------------------------------------------------------------------------------
        /** generates the header for the axis which saves to file
         *  @param alpha ::   onstant string Axis letter that is being used
         *  @param outfile :: File it will save it out to
         *  @param singleSpc :: Constant string for single space
         *  @param fourspc :: Constant string for four spaces
         *  @param nBins ::  Number of bins
         */
        void SaveOpenGenieAscii::writeAxisHeader(const std::string alpha,
                                                 std::ofstream &outfile,
                                                 const std::string singleSpc,
                                                 const std::string fourspc, int nBins) {
            const std::string GXR = "GXRealarray";
            const std::string banknum = "1";
            const std::string twospc = " ";
            
            outfile << twospc << singleSpc << alpha << std::endl;
            outfile << fourspc << GXR << std::endl;
            outfile << fourspc << banknum << std::endl;
            outfile << fourspc << nBins << std::endl;
        }
        
        //-----------------------------------------------------------------------------
        /** Uses AxisHeader and WriteAxisValues to write out file
         *  @param alpha ::   Axis letter that is being used
         *  @param outfile :: File it will save it out to
         *  @param singleSpc :: Constant string for single space
         *  @param fourspc :: Constant string for four spaces
         *  @param nBins ::  number of bins
         *  @param isHistogram ::  If its a histogram
         */
        void SaveOpenGenieAscii::writeToFile(const std::string alpha,
                                             std::ofstream &outfile,
                                             const std::string singleSpc,
                                             const std::string fourspc, int nBins,
                                             bool isHistogram) {
            
            writeAxisHeader(alpha, outfile, singleSpc, fourspc, nBins);
            
            for (int bin = 0; bin < nBins; bin++) {
                if (isHistogram) // bin centres
                {
                    if (bin == 0) {
                        outfile << fourspc;
                    }
                    
                    writeAxisValues(alpha, outfile, bin, singleSpc);
                    
                    if ((bin + 1) % 10 == 0 && bin != (nBins - 1)) {
                        outfile << std::endl << fourspc;
                    }
                }
            }
            outfile << std::endl;
        }
        
        //------------------------------------------------------------------------
        /** Reads if alpha is e then reads the E values accordingly
         *  @param alpha ::   Axis letter that is being used
         *  @param outfile :: File it will save it out to
         *  @param bin :: bin counter which goes through all the bin
         *  @param singleSpc :: Constant string for single space
         */
        void SaveOpenGenieAscii::writeAxisValues(std::string alpha,
                                                 std::ofstream &outfile, int bin,
                                                 const std::string singleSpc) {
            if (alpha == "\"e\"") {
                outfile << ws->readE(0)[bin] << singleSpc;
            }
            if (alpha == "\"x\"") {
                outfile << (ws->readX(0)[bin]) << singleSpc;
            }
            if (alpha == "\"y\"") {
                outfile << ws->readY(0)[bin] << singleSpc;
            }
        }
        
        //-----------------------------------------------------------------------
        /** Reads the sample logs
         *  @param outfile :: File it will save it out to
         *  @param fourspc :: Constant string for four spaces
         */
        void SaveOpenGenieAscii::writeSampleLogs(std::ofstream &outfile,
                                                 std::string fourspc) {
            const std::vector<Property *> &logData = ws->run().getLogData();
            
            // vector
            std::vector<std::string> myVector;
            for (auto log = logData.begin(); log != logData.end(); ++log) {
                std::string name = (*log)->name();
                std::string type = (*log)->type();
                std::string value = (*log)->value();
                
                if (type.std::string::find("vector") &&
                    type.std::string::find("double") != std::string::npos) {
                    
                    auto tsp = ws->run().getTimeSeriesProperty<double>(name);
                    // value = std::to_string(tsp->timeAverageValue());
                    value = boost::lexical_cast<std::string>(tsp->timeAverageValue());
                }
                
                if (type.std::string::find("vector") &&
                    type.std::string::find("int") != std::string::npos) {
                    
                    auto tsp = ws->run().getTimeSeriesProperty<int>(name);
                    value = boost::lexical_cast<std::string>(tsp->timeAverageValue());
                }
                
                if (type.std::string::find("vector") &&
                    type.std::string::find("bool") != std::string::npos) {
                    
                    auto tsp = ws->run().getTimeSeriesProperty<bool>(name);
                    value = boost::lexical_cast<std::string>(tsp->timeAverageValue());
                }
                
                if (type.std::string::find("vector") &&
                    type.std::string::find("char") != std::string::npos) {
                    
                    auto tsp = ws->run().getTimeSeriesProperty<std::string>(name);
                    value = (tsp->lastValue());
                }
                
                if ((type.std::string::find("number") != std::string::npos) ||
                    (type.std::string::find("double") != std::string::npos) ||
                    (type.std::string::find("dbl list") != std::string::npos)) {
                    type = "Float";
                }
                
                if ((type.std::string::find("TimeValueUnit<bool>") != std::string::npos) ||
                    (type.std::string::find("TimeValueUnit<int>") != std::string::npos)) {
                    type = "Integer";
                }
                
                if (type.std::string::find("string") != std::string::npos) {
                    type = "String";
                }
                
                // sort(&logData.begin(), &logData.end());
                
                /*
                 if(name != "x" && name != "y" && name != "e" ) {
                 outfile << "  \"" << name << "\"" << std::endl
                 << fourspc << type << std::endl
                 << fourspc << value << std::endl;
                 } */
                
                if (name != "x" && name != "y" && name != "e") {
                    std::string output = ("  \"" + name + "\"" + "\n" + fourspc + type +
                                          "\n" + fourspc + value + "\n");
                    
                    myVector.push_back(output);
                }
            }
            sort(myVector.begin(), myVector.end());
            
            for (std::vector<std::string>::const_iterator i = myVector.begin();
                 i != myVector.end(); ++i) {
                outfile << *i << ' ';
            }
        }
        
    } // namespace DataHandling
} // namespace Mantid