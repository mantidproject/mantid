// SaveSNSNexus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, STFC eScience. Modified to fit with SaveSNSNexusProcessed
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/SaveSNSNexus.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include "Poco/File.h"
//#include "hdf5.h" //This is troublesome on multiple platforms.

#include "stdlib.h"
#include <string.h>
#include <time.h>

namespace Mantid
{
namespace NeXus
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(SaveSNSNexus)

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;
  using namespace Geometry;

  /// Empty default constructor
  SaveSNSNexus::SaveSNSNexus() : Algorithm() {}

  /** Initialisation method.
   *
   */
  void SaveSNSNexus::init()
  {
    // Declare required parameters, filename with ext {.nx,.nx5,xml} and input workspac
    std::vector<std::string> exts;
    exts.push_back(".nxs");

    declareProperty(new FileProperty("InputFilename", "", FileProperty::Load, exts),
          "The name of the original Nexus file for this data,\n"
          "as a full or relative path");

    declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace", "", Direction::Input),
        "Name of the workspace to be saved");

    declareProperty(new FileProperty("OutputFilename", "", FileProperty::Save, exts),
          "The name of the Nexus file to write, as a full or relative\n"
          "path");

  }


//  /** Execute the algorithm.
//   *
//   *  @throw runtime_error Thrown if algorithm cannot execute
//   */
//  void SaveSNSNexus::exec()
//  {
//    throw std::runtime_error("Temporarily disabled because it does not work on RHEL5.\n");
//  }


  //------------------------------------------------------------------------
  /** Append to current_path */
  int SaveSNSNexus::add_path(const char* path)
  {
    int i;
    i = strlen(current_path);
    sprintf(current_path + i, "/%s", path);
    return 0;
  }


  //------------------------------------------------------------------------
  /** Remove the last part of the path */
  int SaveSNSNexus::remove_path(const char* path)
  {
    char *tstr;
    tstr = strrchr(current_path, '/');
    if (tstr != NULL && !strcmp(path, tstr+1))
    {
      *tstr = '\0';
    }
    else
    {
      printf("path error\n");
    }
    return 0;
  }





  //------------------------------------------------------------------------
  /** Performs the copying from the input to the output file,
   *  while modifying the data and time_of_flight fields.
   */
  int SaveSNSNexus::copy_file(const char* inFile, int nx_read_access, const char* outFile, int nx_write_access)
  {
    int i, nx_is_definition = 0;
    char* tstr;
    links_count = 0;
    current_path[0] = '\0';
    NXlink link;

    /* Open NeXus input file and NeXus output file */
    if (NXopen (inFile, nx_read_access, &inId) != NX_OK) {
      printf ("NX_ERROR: Can't open %s\n", inFile);
      return NX_ERROR;
    }

    if (NXopen (outFile, nx_write_access, &outId) != NX_OK) {
      printf ("NX_ERROR: Can't open %s\n", outFile);
      return NX_ERROR;
    }

    /* Output global attributes */
    if (WriteAttributes (nx_is_definition) != NX_OK)
    {
      return NX_ERROR;
    }
    /* Recursively cycle through the groups printing the contents */
    if (WriteGroup (nx_is_definition) != NX_OK)
    {
      return NX_ERROR;
    }
    /* close input */
    if (NXclose (&inId) != NX_OK)
    {
      return NX_ERROR;
    }

    //HDF5 only
    {
      /* now create any required links */
      for(i=0; i<links_count; i++)
      {
        if (NXopenpath(outId, links_to_make[i].to) != NX_OK) return NX_ERROR;
        if (NXgetdataID(outId, &link) == NX_OK  || NXgetgroupID(outId, &link) == NX_OK)
        {
          if (NXopenpath(outId, links_to_make[i].from) != NX_OK) return NX_ERROR;
          tstr = strrchr(links_to_make[i].to, '/');
          if (!strcmp(links_to_make[i].name, tstr+1))
          {
            if (NXmakelink(outId, &link) != NX_OK) return NX_ERROR;
          }
          else
          {
            if (NXmakenamedlink(outId, links_to_make[i].name, &link) != NX_OK) return NX_ERROR;
          }
        }
        else
        {
          return NX_ERROR;
        }
      }
    }
    /* Close the input and output files */
    if (NXclose (&outId) != NX_OK)
    {
      return NX_ERROR;
    }
    return NX_OK;
  }

  //------------------------------------------------------------------------
  /** Prints the contents of each group as XML tags and values */
  int SaveSNSNexus::WriteGroup (int is_definition)
  {
    int status, dataType, dataRank, dataDimensions[NX_MAXRANK];
    NXname name, theClass;
    void *dataBuffer;
    NXlink link;

    do
    {
      status = NXgetnextentry (inId, name, theClass, &dataType);
//      std::cout << name << "(" << theClass << ")\n";

      if (status == NX_ERROR) return NX_ERROR;
      if (status == NX_OK)
      {
        if (!strncmp(theClass,"NX",2))
        {
          if (NXopengroup (inId, name, theClass) != NX_OK) return NX_ERROR;
          add_path(name);

          if (NXgetgroupID(inId, &link) != NX_OK) return NX_ERROR;
          if (!strcmp(current_path, link.targetPath))
          {
            // Create a copy of the group
            if (NXmakegroup (outId, name, theClass) != NX_OK) return NX_ERROR;
            if (NXopengroup (outId, name, theClass) != NX_OK) return NX_ERROR;
            if (WriteAttributes (is_definition) != NX_OK) return NX_ERROR;
            if (WriteGroup (is_definition) != NX_OK) return NX_ERROR;
            remove_path(name);
          }
          else
          {
            remove_path(name);
            strcpy(links_to_make[links_count].from, current_path);
            strcpy(links_to_make[links_count].to, link.targetPath);
            strcpy(links_to_make[links_count].name, name);
            links_count++;
            if (NXclosegroup (inId) != NX_OK) return NX_ERROR;
          }
        }
        else if (!strncmp(theClass,"SDS",3))
        {
          add_path(name);
          if (NXopendata (inId, name) != NX_OK) return NX_ERROR;
          if (NXgetdataID(inId, &link) != NX_OK) return NX_ERROR;

          std::string data_label(name);

          if (!strcmp(current_path, link.targetPath))
          {
            // Look for the bank name
            std::string path(current_path);
            std::string bank("");

            size_t a = path.rfind('/');
            if (a != std::string::npos && a > 0)
            {
              size_t b = path.rfind('/',a-1);
              if (b != std::string::npos && (b < a) && (a-b-1) > 0)
              {
                bank = path.substr(b+1,a-b-1);
                //std::cout << current_path << ":bank " << bank << "\n";
              }
            }

            //---------------------------------------------------------------------------------------
            if (data_label=="data" && (bank != ""))
            {
              if (NXgetinfo (inId, &dataRank, dataDimensions, &dataType) != NX_OK) return NX_ERROR;

              // Get the rectangular detector
              IComponent_sptr det_comp = inputWorkspace->getInstrument()->getComponentByName( std::string(bank) );
              boost::shared_ptr<RectangularDetector> det = boost::dynamic_pointer_cast<RectangularDetector>(det_comp);
              if (!det)
              {
                g_log.information() << "Detector '" + bank + "' not found, or it is not a rectangular detector!\n";
                //Just copy that then.
                if (NXmalloc (&dataBuffer, dataRank, dataDimensions, dataType) != NX_OK) return NX_ERROR;
                if (NXgetdata (inId, dataBuffer)  != NX_OK) return NX_ERROR;
                if (NXmakedata (outId, name, dataType, dataRank, dataDimensions) != NX_OK) return NX_ERROR;
                if (NXopendata (outId, name) != NX_OK) return NX_ERROR;
                if (WriteAttributes (is_definition) != NX_OK) return NX_ERROR;
                if (NXputdata (outId, dataBuffer) != NX_OK) return NX_ERROR;
                if (NXfree((void**)&dataBuffer) != NX_OK) return NX_ERROR;
                if (NXclosedata (outId) != NX_OK) return NX_ERROR;
              }
              else
              {
                //YES it is a rectangular detector.

                // Dimension 0 = the X pixels
                dataDimensions[0] = det->xpixels();
                // Dimension 1 = the Y pixels
                dataDimensions[1] = det->ypixels();
                // Dimension 2 = time of flight bins
                dataDimensions[2] = inputWorkspace->blocksize();

                std::cout << "RectangularDetector " << det->getName() << " being copied. Dimensions : " << dataDimensions[0] << ", " << dataDimensions[1] << ", " << dataDimensions[2] << ".\n";

                // Make a buffer of floats will all the counts in that bank.
                float * data;
                data = new float[dataDimensions[0]*dataDimensions[1]*dataDimensions[2]];
                // Here's one with the errors
                float * errors;
                errors = new float[dataDimensions[0]*dataDimensions[1]*dataDimensions[2]];

                for (int x = 0; x < det->xpixels(); x++)
                {
                  for (int y = 0; y < det->ypixels(); y++)
                  {
                    //Get the workspace index for the detector ID at this spot
                    int wi;
                    try
                    {
                      wi = (*map)[ det->getAtXY(x,y)->getID() ];
                      const MantidVec & Y = inputWorkspace->readY(wi);
                      const MantidVec & E = inputWorkspace->readE(wi);
                      // Offset into array.
                      int index = x*dataDimensions[1]*dataDimensions[2] + y*dataDimensions[2];
                      // Save in the float array
                      for (int i=0; i < static_cast<int>(Y.size()); i++)
                      {
                        data[index+i] = Y[i];
                        errors[index+i] = E[i];
                      }
                    }
                    catch (...)
                    {
                      std::cout << "Error finding " << bank << " x " << x << " y " << y << "\n";
                    }
                  }
                }

                if (NXmakedata (outId, name, NX_FLOAT32, dataRank, dataDimensions) != NX_OK) return NX_ERROR;
                if (NXopendata (outId, name) != NX_OK) return NX_ERROR;
                if (WriteAttributes (is_definition) != NX_OK) return NX_ERROR;

                // Add an attribute called "errors" with value = the name of the data_errors field.
                NXname attrName = "errors";
                std::string attrBuffer = "data_errors";
                if (NXputattr (outId, attrName, (void *) attrBuffer.c_str(), attrBuffer.size(), NX_CHAR) != NX_OK) return NX_ERROR;
                if (NXputdata (outId, data) != NX_OK) return NX_ERROR;

                // ----- Save the data_errors field -------
                NXname errors_name = "data_errors";
                if (NXmakedata (outId, errors_name, NX_FLOAT32, dataRank, dataDimensions) != NX_OK) return NX_ERROR;
                if (NXopendata (outId, errors_name) != NX_OK) return NX_ERROR;
                if (NXputdata (outId, errors) != NX_OK) return NX_ERROR;

                if (NXclosedata (outId) != NX_OK) return NX_ERROR;

                delete [] data;
              }
            }
            //---------------------------------------------------------------------------------------
            else if (data_label=="time_of_flight" && (bank != ""))
            {
              // Get the original info
              if (NXgetinfo (inId, &dataRank, dataDimensions, &dataType) != NX_OK) return NX_ERROR;

              // Get the X bins
              const MantidVec & X = inputWorkspace->readX(0);
              // 1 dimension, with that number of bin boundaries
              dataDimensions[0] = X.size();
              // The output TOF axis will be whatever size in the workspace.
              float       *tof_data;                /* pointer to data buffer to write */
              tof_data = new float[dataDimensions[0]];

              // And fill it with the X data
              for (size_t i=0; i < X.size(); i++)
                tof_data[i] = X[i];

              if (NXmakedata (outId, name, dataType, dataRank, dataDimensions) != NX_OK) return NX_ERROR;
              if (NXopendata (outId, name) != NX_OK) return NX_ERROR;
              if (WriteAttributes (is_definition) != NX_OK) return NX_ERROR;
              if (NXputdata (outId, tof_data) != NX_OK) return NX_ERROR;
              delete [] tof_data;
              if (NXclosedata (outId) != NX_OK) return NX_ERROR;

            }

            //---------------------------------------------------------------------------------------
            else
            {
              //Everything else gets copies
              if (NXgetinfo (inId, &dataRank, dataDimensions, &dataType) != NX_OK) return NX_ERROR;
              if (NXmalloc (&dataBuffer, dataRank, dataDimensions, dataType) != NX_OK) return NX_ERROR;
              if (NXgetdata (inId, dataBuffer)  != NX_OK) return NX_ERROR;
              if (NXmakedata (outId, name, dataType, dataRank, dataDimensions) != NX_OK) return NX_ERROR;
              if (NXopendata (outId, name) != NX_OK) return NX_ERROR;
              if (WriteAttributes (is_definition) != NX_OK) return NX_ERROR;
              if (NXputdata (outId, dataBuffer) != NX_OK) return NX_ERROR;
              if (NXfree((void**)&dataBuffer) != NX_OK) return NX_ERROR;
              if (NXclosedata (outId) != NX_OK) return NX_ERROR;
            }

            remove_path(name);
          }
          else
          {
            //Make a link
            remove_path(name);
            strcpy(links_to_make[links_count].from, current_path);
            strcpy(links_to_make[links_count].to, link.targetPath);
            strcpy(links_to_make[links_count].name, name);
            links_count++;
          }
          if (NXclosedata (inId) != NX_OK) return NX_ERROR;
        }
      }
      else if (status == NX_EOD) {
        if (NXclosegroup (inId) != NX_OK) return NX_ERROR;
        if (NXclosegroup (outId) != NX_OK) return NX_ERROR;
        return NX_OK;
      }
    } while (status == NX_OK);
    return NX_OK;
  }



  //------------------------------------------------------------------------
  /** Copy the attributes from input to output */
  int SaveSNSNexus::WriteAttributes (int is_definition)
  {
    int status, i, attrLen, attrType;
    NXname attrName;
    void *attrBuffer;

    i = 0;
    do {
      status = NXgetnextattr (inId, attrName, &attrLen, &attrType);
      if (status == NX_ERROR) return NX_ERROR;
      if (status == NX_OK) {
        if (strcmp(attrName, "NeXus_version") && strcmp(attrName, "XML_version") &&
            strcmp(attrName, "HDF_version") && strcmp(attrName, "HDF5_Version") &&
            strcmp(attrName, "file_name") && strcmp(attrName, "file_time")) {
          attrLen++; /* Add space for string termination */
          if (NXmalloc((void**)&attrBuffer, 1, &attrLen, attrType) != NX_OK) return NX_ERROR;
          if (NXgetattr (inId, attrName, attrBuffer, &attrLen , &attrType) != NX_OK) return NX_ERROR;
          if (NXputattr (outId, attrName, attrBuffer, attrLen , attrType) != NX_OK) return NX_ERROR;
          if (NXfree((void**)&attrBuffer) != NX_OK) return NX_ERROR;
        }
        i++;
      }
    } while (status != NX_EOD);
    return NX_OK;
  }






  //------------------------------------------------------------------------
  /** Execute the algorithm.
   *
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void SaveSNSNexus::exec()
  {
    // Retrieve the filename from the properties
    m_inputFilename = getPropertyValue("InputFileName");
    m_inputWorkspaceName = getPropertyValue("InputWorkspace");
    m_outputFilename = getPropertyValue("OutputFileName");

    inputWorkspace = getProperty("InputWorkspace");

    // We'll need to get workspace indices
    map = inputWorkspace->getDetectorIDToWorkspaceIndexMap( false );

    this->copy_file(m_inputFilename.c_str(),  NXACC_READ, m_outputFilename.c_str(),  NXACC_CREATE5);

    // Free map memory
    delete map;

    return;
  }


} // namespace NeXus
} // namespace Mantid
