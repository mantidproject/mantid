// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// SaveToSNSHistogramNexus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, STFC eScience. Modified to fit with
// SaveToSNSHistogramNexusProcessed
#include "MantidDataHandling/SaveToSNSHistogramNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/Timer.h"

#include <Poco/File.h>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <numeric>

#include <cstdlib>
#include <cstring>
#include <ctime>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveToSNSHistogramNexus)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;

/// Empty default constructor
SaveToSNSHistogramNexus::SaveToSNSHistogramNexus()
    : Algorithm(), m_progress(), m_compress(false), links_count(0), inId(),
      outId() {}

/** Initialisation method.
 *
 */
void SaveToSNSHistogramNexus::init() {
  // Declare required parameters, filename with ext {.nx,.nx5,xml} and input
  // workspac
  std::initializer_list<std::string> exts = {".nxs"};

  declareProperty(Kernel::make_unique<FileProperty>("InputFilename", "",
                                                    FileProperty::Load, exts),
                  "The name of the original Nexus file for this data,\n"
                  "as a full or relative path");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Name of the workspace to be saved");

  declareProperty(Kernel::make_unique<FileProperty>("OutputFilename", "",
                                                    FileProperty::Save, exts),
                  "The name of the Nexus file to write, as a full or relative\n"
                  "path");

  declareProperty(
      make_unique<PropertyWithValue<bool>>("Compress", false, Direction::Input),
      "Will the output NXS file data be compressed?");
}

//  /** Execute the algorithm.
//   *
//   *  @throw runtime_error Thrown if algorithm cannot execute
//   */
//  void SaveToSNSHistogramNexus::exec()
//  {
//    throw std::runtime_error("Temporarily disabled because it does not work on
//    RHEL5.\n");
//  }

//------------------------------------------------------------------------
/** Append to current_path */
int SaveToSNSHistogramNexus::add_path(const char *path) {
  size_t i;
  i = strlen(current_path);
  sprintf(current_path + i, "/%s", path);
  return 0;
}

//------------------------------------------------------------------------
/** Remove the last part of the path */
int SaveToSNSHistogramNexus::remove_path(const char *path) {
  char *tstr;
  tstr = strrchr(current_path, '/');
  if (tstr != nullptr && !strcmp(path, tstr + 1)) {
    *tstr = '\0';
  } else {
    printf("path error\n");
  }
  return 0;
}

//------------------------------------------------------------------------
/** Performs the copying from the input to the output file,
 *  while modifying the data and time_of_flight fields.
 */
int SaveToSNSHistogramNexus::copy_file(const char *inFile, int nx__access,
                                       const char *outFile,
                                       int nx_write_access) {
  int nx_is_definition = 0;
  links_count = 0;
  current_path[0] = '\0';
  NXlink link;

  /* Open NeXus input file and NeXus output file */
  if (NXopen(inFile, nx__access, &inId) != NX_OK) {
    printf("NX_ERROR: Can't open %s\n", inFile);
    return NX_ERROR;
  }

  if (NXopen(outFile, nx_write_access, &outId) != NX_OK) {
    printf("NX_ERROR: Can't open %s\n", outFile);
    return NX_ERROR;
  }

  /* Output global attributes */
  if (WriteAttributes(nx_is_definition) != NX_OK) {
    return NX_ERROR;
  }
  /* Recursively cycle through the groups printing the contents */
  if (WriteGroup(nx_is_definition) != NX_OK) {
    return NX_ERROR;
  }
  /* close input */
  if (NXclose(&inId) != NX_OK) {
    return NX_ERROR;
  }

  // HDF5 only
  {
    /* now create any required links */
    for (int i = 0; i < links_count; i++) {
      if (NXopenpath(outId, links_to_make[i].to) != NX_OK)
        return NX_ERROR;
      if (NXgetdataID(outId, &link) == NX_OK ||
          NXgetgroupID(outId, &link) == NX_OK) {
        if (NXopenpath(outId, links_to_make[i].from) != NX_OK)
          return NX_ERROR;
        char *tstr = strrchr(links_to_make[i].to, '/');
        if (!strcmp(links_to_make[i].name, tstr + 1)) {
          if (NXmakelink(outId, &link) != NX_OK)
            return NX_ERROR;
        } else {
          if (NXmakenamedlink(outId, links_to_make[i].name, &link) != NX_OK)
            return NX_ERROR;
        }
      } else {
        return NX_ERROR;
      }
    }
  }
  /* Close the input and output files */
  if (NXclose(&outId) != NX_OK) {
    return NX_ERROR;
  }
  return NX_OK;
}

//------------------------------------------------------------------------
/** Utility function to write out the
 * data or errors to a field in the group.
 *
 * @param det :: rectangular detector being written
 * @param x_pixel_slab :: size of a slab to write, in number of X pixels.
 *ignored if doBoth
 * @param field_name :: "data" field name
 * @param errors_field_name :: "errors" field name.
 * @param doErrors :: set true if you are writing the errors field this time.
 *field_name should be the "errors" field name
 * @param doBoth :: do both data and errors at once, no slabbing.
 * @param is_definition ::
 * @param bank :: name of the bank being written.
 * @return error code
 */
int SaveToSNSHistogramNexus::WriteOutDataOrErrors(
    Geometry::RectangularDetector_const_sptr det, int x_pixel_slab,
    const char *field_name, const char *errors_field_name, bool doErrors,
    bool doBoth, int is_definition, std::string bank) {
  int dataRank, dataDimensions[NX_MAXRANK];
  int slabDimensions[NX_MAXRANK], slabStartIndices[NX_MAXRANK];

  dataRank = 3;

  // Dimension 0 = the X pixels
  dataDimensions[0] = det->xpixels();
  // Dimension 1 = the Y pixels
  dataDimensions[1] = det->ypixels();
  // Dimension 2 = time of flight bins
  dataDimensions[2] = static_cast<int>(m_inputWorkspace->blocksize());

  // ---- Determine slab size -----
  // Number of pixels to collect in X before slabbing
  slabDimensions[0] = x_pixel_slab;
  slabDimensions[1] = dataDimensions[1];
  slabDimensions[2] = dataDimensions[2];

  if (doBoth)
    slabDimensions[0] = dataDimensions[0];

  std::cout << "RectangularDetector " << det->getName()
            << " being copied. Dimensions : " << dataDimensions[0] << ", "
            << dataDimensions[1] << ", " << dataDimensions[2] << ".\n";

  // ----- Open the data field -----------------------
  if (m_compress) {
    if (NXcompmakedata(outId, field_name, NX_FLOAT32, dataRank, dataDimensions,
                       NX_COMP_LZW, slabDimensions) != NX_OK)
      return NX_ERROR;
  } else {
    if (NXmakedata(outId, field_name, NX_FLOAT32, dataRank, dataDimensions) !=
        NX_OK)
      return NX_ERROR;
  }
  if (NXopendata(outId, field_name) != NX_OK)
    return NX_ERROR;
  if (WriteAttributes(is_definition) != NX_OK)
    return NX_ERROR;
  if (!doErrors) {
    // Add an attribute called "errors" with value = the name of the data_errors
    // field.
    NXname attrName = "errors";
    std::string attrBuffer = errors_field_name;
    if (NXputattr(outId, attrName, attrBuffer.c_str(),
                  static_cast<int>(attrBuffer.size()), NX_CHAR) != NX_OK)
      return NX_ERROR;
  }

  // ---- Errors field -----
  if (doBoth) {
    if (NXclosedata(outId) != NX_OK)
      return NX_ERROR;

    if (m_compress) {
      if (NXcompmakedata(outId, errors_field_name, NX_FLOAT32, dataRank,
                         dataDimensions, NX_COMP_LZW, slabDimensions) != NX_OK)
        return NX_ERROR;
    } else {
      if (NXmakedata(outId, errors_field_name, NX_FLOAT32, dataRank,
                     dataDimensions) != NX_OK)
        return NX_ERROR;
    }
    if (NXopendata(outId, errors_field_name) != NX_OK)
      return NX_ERROR;

    //      NXlink * link = new NXlink;
    //      link->linkType = 1; /* SDS data link */
    //      NXgetdataID(outId, link);
    //      std::string targetPath = "/entry/" + bank + "/" + errors_field_name;
    //      strcpy(link->targetPath, targetPath.c_str());
    //      if (NXmakelink(outId,link) != NX_OK)
    //        g_log.debug() << "Error while making link to " << targetPath <<
    //        '\n';

    if (WriteAttributes(is_definition) != NX_OK)
      return NX_ERROR;
    if (NXclosedata(outId) != NX_OK)
      return NX_ERROR;
  }

  double fillTime = 0;
  double saveTime = 0;

  // Make a buffer of floats will all the counts in that bank.
  auto data =
      new float[slabDimensions[0] * slabDimensions[1] * slabDimensions[2]];

  // Only allocate an array for errors if it is needed
  float *errors = nullptr;
  if (doBoth)
    errors =
        new float[slabDimensions[0] * slabDimensions[1] * slabDimensions[2]];

  for (int x = 0; x < det->xpixels(); x++) {
    // Which slab are we in?
    int slabnum = x / x_pixel_slab;

    // X index into the slabbed output array
    int slabx = x % x_pixel_slab;

    Timer tim1;
    int ypixels = static_cast<int>(det->ypixels());

    PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWorkspace))
    for (int y = 0; y < ypixels; y++) {
      PARALLEL_START_INTERUPT_REGION
      // Get the workspace index for the detector ID at this spot
      size_t wi = 0;
      try {
        wi = m_map.find(det->getAtXY(x, y)->getID())->second;
      } catch (...) {
        std::cout << "Error finding " << bank << " x " << x << " y " << y
                  << "\n";
      }

      // Offset into array.
      size_t index = size_t(slabx) * size_t(dataDimensions[1]) *
                         size_t(dataDimensions[2]) +
                     size_t(y) * size_t(dataDimensions[2]);

      const auto &Y = m_inputWorkspace->y(wi);
      const auto &E = m_inputWorkspace->e(wi);

      for (size_t i = 0; i < Y.size(); ++i) {
        if (doErrors) {
          data[i + index] = static_cast<float>(E[i]);
        } else {
          data[i + index] = static_cast<float>(Y[i]);
          if (doBoth) {
            errors[i + index] = static_cast<float>(E[i]);
          }
        }
      }

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    fillTime += tim1.elapsed();

    // Is this the last pixel in the slab?
    if (!doBoth && (x % x_pixel_slab == x_pixel_slab - 1)) {
      Timer tim2;
      // std::cout << "starting slab " << x << "\n";
      // This is where the slab is in the greater data array.
      slabStartIndices[0] = slabnum * x_pixel_slab;
      slabStartIndices[1] = 0;
      slabStartIndices[2] = 0;
      if (NXputslab(outId, data, slabStartIndices, slabDimensions) != NX_OK)
        return NX_ERROR;
      saveTime += tim2.elapsed();

      std::ostringstream mess;
      mess << det->getName() << ", " << field_name << " slab " << slabnum
           << " of " << det->xpixels() / x_pixel_slab;
      this->m_progress->reportIncrement(x_pixel_slab * det->ypixels(),
                                        mess.str());
    }

  } // X loop

  if (doBoth) {
    bool returnerror = false;

    Timer tim2;
    if (NXopendata(outId, field_name) != NX_OK)
      returnerror = true;
    else if (NXputdata(outId, data) != NX_OK)
      returnerror = true;
    else if (NXclosedata(outId) != NX_OK)
      returnerror = true;
    else {
      this->m_progress->reportIncrement(det->xpixels() * det->ypixels() * 1,
                                        det->getName() + " data");

      if (NXopendata(outId, errors_field_name) != NX_OK)
        returnerror = true;
      else if (NXputdata(outId, errors) != NX_OK)
        returnerror = true;
      else if (NXclosedata(outId) != NX_OK)
        returnerror = true;
      else {
        this->m_progress->reportIncrement(det->xpixels() * det->ypixels() * 1,
                                          det->getName() + " errors");
        saveTime += tim2.elapsed();
      }
    }

    if (returnerror) {
      delete[] data;
      delete[] errors;

      return NX_ERROR;
    }

  } else {
    if (NXclosedata(outId) != NX_OK) {
      delete[] data;
      return NX_ERROR;
    }
  }

  std::cout << "Filling out " << det->getName() << " took " << fillTime
            << " sec.\n";
  std::cout << "Saving      " << det->getName() << " took " << saveTime
            << " sec.\n";

  delete[] data;
  if (doBoth)
    delete[] errors;

  return NX_OK;
}

//=================================================================================================
/** Write the group labeled "data"
 *
 * @param bank :: name of the bank
 * @param is_definition
 * @return error code
 */
int SaveToSNSHistogramNexus::WriteDataGroup(std::string bank,
                                            int is_definition) {
  int dataType, dataRank, dataDimensions[NX_MAXRANK];
  NXname name;
  void *dataBuffer;

  if (NXgetinfo(inId, &dataRank, dataDimensions, &dataType) != NX_OK)
    return NX_ERROR;

  // Get the rectangular detector
  IComponent_const_sptr det_comp =
      m_inputWorkspace->getInstrument()->getComponentByName(std::string(bank));
  RectangularDetector_const_sptr det =
      boost::dynamic_pointer_cast<const RectangularDetector>(det_comp);
  if (!det) {
    g_log.information()
        << "Detector '" + bank +
               "' not found, or it is not a rectangular detector!\n";
    // Just copy that then.
    if (NXmalloc(&dataBuffer, dataRank, dataDimensions, dataType) != NX_OK)
      return NX_ERROR;
    if (NXgetdata(inId, dataBuffer) != NX_OK)
      return NX_ERROR;
    if (NXcompmakedata(outId, name, dataType, dataRank, dataDimensions,
                       NX_COMP_LZW, dataDimensions) != NX_OK)
      return NX_ERROR;
    if (NXopendata(outId, name) != NX_OK)
      return NX_ERROR;
    if (WriteAttributes(is_definition) != NX_OK)
      return NX_ERROR;
    if (NXputdata(outId, dataBuffer) != NX_OK)
      return NX_ERROR;
    if (NXfree(&dataBuffer) != NX_OK)
      return NX_ERROR;
    if (NXclosedata(outId) != NX_OK)
      return NX_ERROR;
  } else {
    // YES it is a rectangular detector.

    // --- Memory requirements ----
    size_t memory_required = size_t(det->xpixels() * det->ypixels()) *
                             size_t(m_inputWorkspace->blocksize()) * 2 *
                             sizeof(float);
    Kernel::MemoryStats mem;
    mem.update();
    size_t memory_available = mem.availMem() * 1024;

    std::cout << "Memory available: " << memory_available / 1024 << " kb. ";
    std::cout << "Memory required: " << memory_required / 1024 << " kb. ";

    // Give a 50% margin of error in allocating the memory
    memory_available = memory_available / 2;
    if (memory_available > static_cast<size_t>(5e9))
      memory_available = static_cast<size_t>(5e9);

    if (memory_available < memory_required) {
      // Compute how large of a slab you can still use.
      int x_slab;
      x_slab = static_cast<int>(
          memory_available /
          (det->ypixels() * m_inputWorkspace->blocksize() * 2 * sizeof(float)));
      if (x_slab <= 0)
        x_slab = 1;
      // Look for a slab size that evenly divides the # of pixels.
      while (x_slab > 1) {
        if ((det->xpixels() % x_slab) == 0)
          break;
        x_slab--;
      }

      std::cout << "Saving in slabs of " << x_slab << " X pixels.\n";
      if (this->WriteOutDataOrErrors(det, x_slab, "data", "data_errors", false,
                                     false, is_definition, bank) != NX_OK)
        return NX_ERROR;
      if (this->WriteOutDataOrErrors(det, x_slab, "errors", "", true, false,
                                     is_definition, bank) != NX_OK)
        return NX_ERROR;
    } else {
      std::cout << "Saving in one block.\n";
      if (this->WriteOutDataOrErrors(det, det->xpixels(), "data", "data_errors",
                                     false, true, is_definition, bank) != NX_OK)
        return NX_ERROR;
    }
  }

  return NX_OK;
}

//------------------------------------------------------------------------
/** Prints the contents of each group as XML tags and values */
int SaveToSNSHistogramNexus::WriteGroup(int is_definition) {
  int status, dataType, dataRank, dataDimensions[NX_MAXRANK];
  NXname name, theClass;
  void *dataBuffer;
  NXlink link;

  do {
    status = NXgetnextentry(inId, name, theClass, &dataType);
    //      std::cout << name << "(" << theClass << ")\n";

    if (status == NX_ERROR)
      return NX_ERROR;
    if (status == NX_OK) {
      if (!strncmp(theClass, "NX", 2)) {
        if (NXopengroup(inId, name, theClass) != NX_OK)
          return NX_ERROR;
        add_path(name);

        if (NXgetgroupID(inId, &link) != NX_OK)
          return NX_ERROR;
        if (!strcmp(current_path, link.targetPath)) {
          // Create a copy of the group
          if (NXmakegroup(outId, name, theClass) != NX_OK)
            return NX_ERROR;
          if (NXopengroup(outId, name, theClass) != NX_OK)
            return NX_ERROR;
          if (WriteAttributes(is_definition) != NX_OK)
            return NX_ERROR;
          if (WriteGroup(is_definition) != NX_OK)
            return NX_ERROR;
          remove_path(name);
        } else {
          remove_path(name);
          strcpy(links_to_make[links_count].from, current_path);
          strcpy(links_to_make[links_count].to, link.targetPath);
          strcpy(links_to_make[links_count].name, name);
          links_count++;
          if (NXclosegroup(inId) != NX_OK)
            return NX_ERROR;
        }
      } else if (!strncmp(theClass, "SDS", 3)) {
        add_path(name);
        if (NXopendata(inId, name) != NX_OK)
          return NX_ERROR;
        if (NXgetdataID(inId, &link) != NX_OK)
          return NX_ERROR;

        std::string data_label(name);

        if (!strcmp(current_path, link.targetPath)) {
          // Look for the bank name
          std::string path(current_path);
          std::string bank;

          size_t a = path.rfind('/');
          if (a != std::string::npos && a > 0) {
            size_t b = path.rfind('/', a - 1);
            if (b != std::string::npos && (b < a) && (a - b - 1) > 0) {
              bank = path.substr(b + 1, a - b - 1);
              // std::cout << current_path << ":bank " << bank << "\n";
            }
          }

          //---------------------------------------------------------------------------------------
          if (data_label == "data" && (!bank.empty())) {
            if (this->WriteDataGroup(bank, is_definition) != NX_OK)
              return NX_ERROR;
            ;
          }
          //---------------------------------------------------------------------------------------
          else if (data_label == "time_of_flight" && (!bank.empty())) {
            // Get the original info
            if (NXgetinfo(inId, &dataRank, dataDimensions, &dataType) != NX_OK)
              return NX_ERROR;

            // Get the X bins
            const auto &X = m_inputWorkspace->y(0);
            // 1 dimension, with that number of bin boundaries
            dataDimensions[0] = static_cast<int>(X.size());
            // The output TOF axis will be whatever size in the workspace.
            std::vector<float> tof_data(dataDimensions[0]);

            // And fill it with the X data
            std::transform(X.cbegin(), X.cend(), tof_data.begin(),
                           [](double x) { return static_cast<float>(x); });

            if (NXcompmakedata(outId, name, dataType, dataRank, dataDimensions,
                               NX_COMP_LZW, dataDimensions) != NX_OK)
              return NX_ERROR;
            if (NXopendata(outId, name) != NX_OK)
              return NX_ERROR;
            if (WriteAttributes(is_definition) != NX_OK)
              return NX_ERROR;
            if (NXputdata(outId, tof_data.data()) != NX_OK)
              return NX_ERROR;
            if (NXclosedata(outId) != NX_OK)
              return NX_ERROR;

          }

          //---------------------------------------------------------------------------------------
          else {
            // Everything else gets copies
            if (NXgetinfo(inId, &dataRank, dataDimensions, &dataType) != NX_OK)
              return NX_ERROR;
            if (NXmalloc(&dataBuffer, dataRank, dataDimensions, dataType) !=
                NX_OK)
              return NX_ERROR;
            if (NXgetdata(inId, dataBuffer) != NX_OK)
              return NX_ERROR;
            if (NXcompmakedata(outId, name, dataType, dataRank, dataDimensions,
                               NX_COMP_LZW, dataDimensions) != NX_OK)
              return NX_ERROR;
            if (NXopendata(outId, name) != NX_OK)
              return NX_ERROR;
            if (WriteAttributes(is_definition) != NX_OK)
              return NX_ERROR;
            if (NXputdata(outId, dataBuffer) != NX_OK)
              return NX_ERROR;
            if (NXfree(&dataBuffer) != NX_OK)
              return NX_ERROR;
            if (NXclosedata(outId) != NX_OK)
              return NX_ERROR;
          }

          remove_path(name);
        } else {
          // Make a link
          remove_path(name);
          strcpy(links_to_make[links_count].from, current_path);
          strcpy(links_to_make[links_count].to, link.targetPath);
          strcpy(links_to_make[links_count].name, name);
          links_count++;
        }
        if (NXclosedata(inId) != NX_OK)
          return NX_ERROR;
      }
    } else if (status == NX_EOD) {
      if (NXclosegroup(inId) != NX_OK)
        return NX_ERROR;
      if (NXclosegroup(outId) != NX_OK)
        return NX_ERROR;
      return NX_OK;
    }
  } while (status == NX_OK);
  return NX_OK;
}

//------------------------------------------------------------------------
/** Copy the attributes from input to output */
int SaveToSNSHistogramNexus::WriteAttributes(int is_definition) {
  (void)is_definition;

  int status, attrLen, attrType;
#ifndef NEXUS43
  int rank;
  int dims[4];
#endif
  NXname attrName;
  void *attrBuffer;

  std::array<const char *, 6> attrs = {{"NeXus_version", "XML_version",
                                        "HDF_version", "HDF5_Version",
                                        "file_name", "file_time"}};

  do {
#ifdef NEXUS43
    status = NXgetnextattr(inId, attrName, &attrLen, &attrType);
#else
    status = NXgetnextattra(inId, attrName, &rank, dims, &attrType);
#endif
    if (status == NX_ERROR)
      return NX_ERROR;
    if (status == NX_OK) {
#ifndef NEXUS43
      if (rank != 1)
        return NX_ERROR;
      attrLen = dims[0];
#endif
      if (std::none_of(attrs.cbegin(), attrs.cend(),
                       [&attrName](const char *name) {
                         return strcmp(attrName, name) == 0;
                       })) {
        attrLen++; /* Add space for string termination */
        if (NXmalloc(&attrBuffer, 1, &attrLen, attrType) != NX_OK)
          return NX_ERROR;
        if (NXgetattr(inId, attrName, attrBuffer, &attrLen, &attrType) != NX_OK)
          return NX_ERROR;
        if (NXputattr(outId, attrName, attrBuffer, attrLen, attrType) != NX_OK)
          return NX_ERROR;
        if (NXfree(&attrBuffer) != NX_OK)
          return NX_ERROR;
      }
    }
  } while (status != NX_EOD);
  return NX_OK;
}

void nexus_print_error(void *pD, char *text) {
  (void)pD;
  std::cout << "Nexus Error: " << text << "\n";
}

//------------------------------------------------------------------------
/** Execute the algorithm.
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SaveToSNSHistogramNexus::exec() {
  // NXMSetError(NULL, nexus_print_error);
  NXMEnableErrorReporting();

  // Retrieve the filename from the properties
  m_inputFilename = getPropertyValue("InputFileName");
  m_outputFilename = getPropertyValue("OutputFileName");
  m_compress = getProperty("Compress");

  m_inputWorkspace = getProperty("InputWorkspace");

  // We'll need to get workspace indices
  m_map = m_inputWorkspace->getDetectorIDToWorkspaceIndexMap();

  // Start the progress bar. 3 reports per histogram.
  m_progress = make_unique<Progress>(
      this, 0.0, 1.0, m_inputWorkspace->getNumberHistograms() * 3);

  EventWorkspace_const_sptr eventWorkspace =
      boost::dynamic_pointer_cast<const EventWorkspace>(m_inputWorkspace);
  if (eventWorkspace) {
    eventWorkspace->sortAll(TOF_SORT, m_progress.get());
  }

  int ret;
  ret = this->copy_file(m_inputFilename.c_str(), NXACC_READ,
                        m_outputFilename.c_str(), NXACC_CREATE5);

  if (ret == NX_ERROR)
    throw std::runtime_error("Nexus error while copying the file.");
}

} // namespace DataHandling
} // namespace Mantid
