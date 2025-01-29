// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include <cmath>
#include <memory>
#include <numeric>

#include <cstdlib>
#include <cstring>
#include <ctime>

namespace Mantid::DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveToSNSHistogramNexus)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;

/// Empty default constructor
SaveToSNSHistogramNexus::SaveToSNSHistogramNexus()
    : Algorithm(), m_progress(), m_compress(false), links_count(0), inId(), outId() {}

/** Initialisation method.
 *
 */
void SaveToSNSHistogramNexus::init() {
  // Declare required parameters, filename with ext {.nx,.nx5,xml} and input
  // workspac
  std::initializer_list<std::string> exts = {".nxs"};

  declareProperty(std::make_unique<FileProperty>("InputFilename", "", FileProperty::Load, exts),
                  "The name of the original Nexus file for this data,\n"
                  "as a full or relative path");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Name of the workspace to be saved");

  declareProperty(std::make_unique<FileProperty>("OutputFilename", "", FileProperty::Save, exts),
                  "The name of the Nexus file to write, as a full or relative\n"
                  "path");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("Compress", false, Direction::Input),
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
NXstatus SaveToSNSHistogramNexus::copy_file(const char *inFile, int nx__access, const char *outFile,
                                            int nx_write_access) {
  int nx_is_definition = 0;
  links_count = 0;
  current_path[0] = '\0';
  NXlink link;

  /* Open NeXus input file and NeXus output file */
  if (NXopen(inFile, nx__access, &inId) != NXstatus::OKAY) {
    printf("NX_ERROR: Can't open %s\n", inFile);
    return NXstatus::ERROR;
  }

  if (NXopen(outFile, nx_write_access, &outId) != NXstatus::OKAY) {
    printf("NX_ERROR: Can't open %s\n", outFile);
    return NXstatus::ERROR;
  }

  /* Output global attributes */
  if (WriteAttributes(nx_is_definition) != NXstatus::OKAY) {
    return NXstatus::ERROR;
  }
  /* Recursively cycle through the groups printing the contents */
  if (WriteGroup(nx_is_definition) != NXstatus::OKAY) {
    return NXstatus::ERROR;
  }
  /* close input */
  if (NXclose(&inId) != NXstatus::OKAY) {
    return NXstatus::ERROR;
  }

  // HDF5 only
  {
    /* now create any required links */
    for (int i = 0; i < links_count; i++) {
      if (NXopenpath(outId, links_to_make[i].to) != NXstatus::OKAY)
        return NXstatus::ERROR;
      if (NXgetdataID(outId, &link) == NXstatus::OKAY || NXgetgroupID(outId, &link) == NXstatus::OKAY) {
        if (NXopenpath(outId, links_to_make[i].from) != NXstatus::OKAY)
          return NXstatus::ERROR;
        char const *tstr = strrchr(links_to_make[i].to, '/');
        if (!strcmp(links_to_make[i].name, tstr + 1)) {
          if (NXmakelink(outId, &link) != NXstatus::OKAY)
            return NXstatus::ERROR;
        } else {
          if (NXmakenamedlink(outId, links_to_make[i].name, &link) != NXstatus::OKAY)
            return NXstatus::ERROR;
        }
      } else {
        return NXstatus::ERROR;
      }
    }
  }
  /* Close the input and output files */
  if (NXclose(&outId) != NXstatus::OKAY) {
    return NXstatus::ERROR;
  }
  return NXstatus::OKAY;
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
NXstatus SaveToSNSHistogramNexus::WriteOutDataOrErrors(const Geometry::RectangularDetector_const_sptr &det,
                                                       int x_pixel_slab, const char *field_name,
                                                       const char *errors_field_name, bool doErrors, bool doBoth,
                                                       int is_definition, const std::string &bank) {
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

  std::cout << "RectangularDetector " << det->getName() << " being copied. Dimensions : " << dataDimensions[0] << ", "
            << dataDimensions[1] << ", " << dataDimensions[2] << ".\n";

  // ----- Open the data field -----------------------
  if (m_compress) {
    if (NXcompmakedata(outId, field_name, NXnumtype::FLOAT32, dataRank, dataDimensions, NX_COMP_LZW, slabDimensions) !=
        NXstatus::OKAY)
      return NXstatus::ERROR;
  } else {
    if (NXmakedata(outId, field_name, NXnumtype::FLOAT32, dataRank, dataDimensions) != NXstatus::OKAY)
      return NXstatus::ERROR;
  }
  if (NXopendata(outId, field_name) != NXstatus::OKAY)
    return NXstatus::ERROR;
  if (WriteAttributes(is_definition) != NXstatus::OKAY)
    return NXstatus::ERROR;
  if (!doErrors) {
    // Add an attribute called "errors" with value = the name of the data_errors
    // field.
    NXname attrName = "errors";
    std::string attrBuffer = errors_field_name;
    if (NXputattr(outId, attrName, attrBuffer.c_str(), static_cast<int>(attrBuffer.size()), NXnumtype::CHAR) !=
        NXstatus::OKAY)
      return NXstatus::ERROR;
  }

  // ---- Errors field -----
  if (doBoth) {
    if (NXclosedata(outId) != NXstatus::OKAY)
      return NXstatus::ERROR;

    if (m_compress) {
      if (NXcompmakedata(outId, errors_field_name, NXnumtype::FLOAT32, dataRank, dataDimensions, NX_COMP_LZW,
                         slabDimensions) != NXstatus::OKAY)
        return NXstatus::ERROR;
    } else {
      if (NXmakedata(outId, errors_field_name, NXnumtype::FLOAT32, dataRank, dataDimensions) != NXstatus::OKAY)
        return NXstatus::ERROR;
    }
    if (NXopendata(outId, errors_field_name) != NXstatus::OKAY)
      return NXstatus::ERROR;

    //      NXlink * link = new NXlink;
    //      link->linkType = 1; /* SDS data link */
    //      NXgetdataID(outId, link);
    //      std::string targetPath = "/entry/" + bank + "/" + errors_field_name;
    //      strcpy(link->targetPath, targetPath.c_str());
    //      if (NXmakelink(outId,link) != NX_OK)
    //        g_log.debug() << "Error while making link to " << targetPath <<
    //        '\n';

    if (WriteAttributes(is_definition) != NXstatus::OKAY)
      return NXstatus::ERROR;
    if (NXclosedata(outId) != NXstatus::OKAY)
      return NXstatus::ERROR;
  }

  double fillTime = 0;
  double saveTime = 0;

  // Make a buffer of floats will all the counts in that bank.
  auto data = new float[slabDimensions[0] * slabDimensions[1] * slabDimensions[2]];

  // Only allocate an array for errors if it is needed
  float *errors = nullptr;
  if (doBoth)
    errors = new float[slabDimensions[0] * slabDimensions[1] * slabDimensions[2]];

  for (int x = 0; x < det->xpixels(); x++) {
    // Which slab are we in?
    int slabnum = x / x_pixel_slab;

    // X index into the slabbed output array
    int slabx = x % x_pixel_slab;

    Timer tim1;
    auto ypixels = static_cast<int>(det->ypixels());

    PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWorkspace))
    for (int y = 0; y < ypixels; y++) {
      PARALLEL_START_INTERRUPT_REGION
      // Get the workspace index for the detector ID at this spot
      size_t wi = 0;
      try {
        wi = m_map.find(det->getAtXY(x, y)->getID())->second;
      } catch (...) {
        std::cout << "Error finding " << bank << " x " << x << " y " << y << "\n";
      }

      // Offset into array.
      size_t index =
          size_t(slabx) * size_t(dataDimensions[1]) * size_t(dataDimensions[2]) + size_t(y) * size_t(dataDimensions[2]);

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

      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    fillTime += tim1.elapsed();

    // Is this the last pixel in the slab?
    if (!doBoth && (x % x_pixel_slab == x_pixel_slab - 1)) {
      Timer tim2;
      // std::cout << "starting slab " << x << "\n";
      // This is where the slab is in the greater data array.
      slabStartIndices[0] = slabnum * x_pixel_slab;
      slabStartIndices[1] = 0;
      slabStartIndices[2] = 0;
      if (NXputslab(outId, data, slabStartIndices, slabDimensions) != NXstatus::OKAY)
        return NXstatus::ERROR;
      saveTime += tim2.elapsed();

      std::ostringstream mess;
      mess << det->getName() << ", " << field_name << " slab " << slabnum << " of " << det->xpixels() / x_pixel_slab;
      this->m_progress->reportIncrement(x_pixel_slab * det->ypixels(), mess.str());
    }

  } // X loop

  if (doBoth) {
    bool returnerror = false;

    Timer tim2;
    if (NXopendata(outId, field_name) != NXstatus::OKAY)
      returnerror = true;
    else if (NXputdata(outId, data) != NXstatus::OKAY)
      returnerror = true;
    else if (NXclosedata(outId) != NXstatus::OKAY)
      returnerror = true;
    else {
      this->m_progress->reportIncrement(det->xpixels() * det->ypixels() * 1, det->getName() + " data");

      if (NXopendata(outId, errors_field_name) != NXstatus::OKAY)
        returnerror = true;
      else if (NXputdata(outId, errors) != NXstatus::OKAY)
        returnerror = true;
      else if (NXclosedata(outId) != NXstatus::OKAY)
        returnerror = true;
      else {
        this->m_progress->reportIncrement(det->xpixels() * det->ypixels() * 1, det->getName() + " errors");
        saveTime += tim2.elapsed();
      }
    }

    if (returnerror) {
      delete[] data;
      delete[] errors;

      return NXstatus::ERROR;
    }

  } else {
    if (NXclosedata(outId) != NXstatus::OKAY) {
      delete[] data;
      return NXstatus::ERROR;
    }
  }

  std::cout << "Filling out " << det->getName() << " took " << fillTime << " sec.\n";
  std::cout << "Saving      " << det->getName() << " took " << saveTime << " sec.\n";

  delete[] data;
  if (doBoth)
    delete[] errors;

  return NXstatus::OKAY;
}

//=================================================================================================
/** Write the group labeled "data"
 *
 * @param bank :: name of the bank
 * @param is_definition
 * @return error code
 */
NXstatus SaveToSNSHistogramNexus::WriteDataGroup(const std::string &bank, int is_definition) {
  NXnumtype dataType;
  int dataRank, dataDimensions[NX_MAXRANK];
  void *dataBuffer;

  if (NXgetinfo(inId, &dataRank, dataDimensions, &dataType) != NXstatus::OKAY)
    return NXstatus::ERROR;

  // Get the rectangular detector
  IComponent_const_sptr det_comp = m_inputWorkspace->getInstrument()->getComponentByName(std::string(bank));
  RectangularDetector_const_sptr det = std::dynamic_pointer_cast<const RectangularDetector>(det_comp);
  if (!det) {
    g_log.information() << "Detector '" + bank + "' not found, or it is not a rectangular detector!\n";
    // Just copy that then.
    if (NXmalloc(&dataBuffer, dataRank, dataDimensions, dataType) != NXstatus::OKAY)
      return NXstatus::ERROR;
    if (NXgetdata(inId, dataBuffer) != NXstatus::OKAY)
      return NXstatus::ERROR;
    NXname nxName;
    if (NXcompmakedata(outId, nxName, dataType, dataRank, dataDimensions, NX_COMP_LZW, // cppcheck-suppress uninitvar
                       dataDimensions) != NXstatus::OKAY)
      return NXstatus::ERROR;
    if (NXopendata(outId, nxName) != NXstatus::OKAY)
      return NXstatus::ERROR;
    if (WriteAttributes(is_definition) != NXstatus::OKAY)
      return NXstatus::ERROR;
    if (NXputdata(outId, dataBuffer) != NXstatus::OKAY)
      return NXstatus::ERROR;
    if (NXfree(&dataBuffer) != NXstatus::OKAY)
      return NXstatus::ERROR;
    if (NXclosedata(outId) != NXstatus::OKAY)
      return NXstatus::ERROR;
  } else {
    // YES it is a rectangular detector.

    // --- Memory requirements ----
    size_t memory_required =
        size_t(det->xpixels() * det->ypixels()) * size_t(m_inputWorkspace->blocksize()) * 2 * sizeof(float);
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
      x_slab =
          static_cast<int>(memory_available / (det->ypixels() * m_inputWorkspace->blocksize() * 2 * sizeof(float)));
      if (x_slab <= 0)
        x_slab = 1;
      // Look for a slab size that evenly divides the # of pixels.
      while (x_slab > 1) {
        if ((det->xpixels() % x_slab) == 0)
          break;
        x_slab--;
      }

      std::cout << "Saving in slabs of " << x_slab << " X pixels.\n";
      if (this->WriteOutDataOrErrors(det, x_slab, "data", "data_errors", false, false, is_definition, bank) !=
          NXstatus::OKAY)
        return NXstatus::ERROR;
      if (this->WriteOutDataOrErrors(det, x_slab, "errors", "", true, false, is_definition, bank) != NXstatus::OKAY)
        return NXstatus::ERROR;
    } else {
      std::cout << "Saving in one block.\n";
      if (this->WriteOutDataOrErrors(det, det->xpixels(), "data", "data_errors", false, true, is_definition, bank) !=
          NXstatus::OKAY)
        return NXstatus::ERROR;
    }
  }

  return NXstatus::OKAY;
}

//------------------------------------------------------------------------
/** Prints the contents of each group as XML tags and values */
NXstatus SaveToSNSHistogramNexus::WriteGroup(int is_definition) {
  NXstatus status;
  NXnumtype dataType;
  int dataRank, dataDimensions[NX_MAXRANK];
  NXname nxName, theClass;
  void *dataBuffer;
  NXlink link;

  do {
    status = NXgetnextentry(inId, nxName, theClass, &dataType);
    //      std::cout << name << "(" << theClass << ")\n";

    if (status == NXstatus::ERROR)
      return NXstatus::ERROR;
    if (status == NXstatus::OKAY) {
      if (!strncmp(theClass, "NX", 2)) {
        if (NXopengroup(inId, nxName, theClass) != NXstatus::OKAY)
          return NXstatus::ERROR;
        add_path(nxName);

        if (NXgetgroupID(inId, &link) != NXstatus::OKAY)
          return NXstatus::ERROR;
        if (!strcmp(current_path, link.targetPath)) {
          // Create a copy of the group
          if (NXmakegroup(outId, nxName, theClass) != NXstatus::OKAY)
            return NXstatus::ERROR;
          if (NXopengroup(outId, nxName, theClass) != NXstatus::OKAY)
            return NXstatus::ERROR;
          if (WriteAttributes(is_definition) != NXstatus::OKAY)
            return NXstatus::ERROR;
          if (WriteGroup(is_definition) != NXstatus::OKAY)
            return NXstatus::ERROR;
          remove_path(nxName);
        } else {
          remove_path(nxName);
          strcpy(links_to_make[links_count].from, current_path);
          strcpy(links_to_make[links_count].to, link.targetPath);
          strcpy(links_to_make[links_count].name, nxName);
          links_count++;
          if (NXclosegroup(inId) != NXstatus::OKAY)
            return NXstatus::ERROR;
        }
      } else if (!strncmp(theClass, "SDS", 3)) {
        add_path(nxName);
        if (NXopendata(inId, nxName) != NXstatus::OKAY)
          return NXstatus::ERROR;
        if (NXgetdataID(inId, &link) != NXstatus::OKAY)
          return NXstatus::ERROR;

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

          std::string data_label(nxName);
          //---------------------------------------------------------------------------------------
          if (data_label == "data" && (!bank.empty())) {
            if (this->WriteDataGroup(bank, is_definition) != NXstatus::OKAY)
              return NXstatus::ERROR;
            ;
          }
          //---------------------------------------------------------------------------------------
          else if (data_label == "time_of_flight" && (!bank.empty())) {
            // Get the original info
            if (NXgetinfo(inId, &dataRank, dataDimensions, &dataType) != NXstatus::OKAY)
              return NXstatus::ERROR;

            // Get the X bins
            const auto &X = m_inputWorkspace->y(0);
            // 1 dimension, with that number of bin boundaries
            dataDimensions[0] = static_cast<int>(X.size());
            // The output TOF axis will be whatever size in the workspace.
            std::vector<float> tof_data(dataDimensions[0]);

            // And fill it with the X data
            std::transform(X.cbegin(), X.cend(), tof_data.begin(), [](double x) { return static_cast<float>(x); });

            if (NXcompmakedata(outId, nxName, dataType, dataRank, dataDimensions, NX_COMP_LZW, dataDimensions) !=
                NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXopendata(outId, nxName) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (WriteAttributes(is_definition) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXputdata(outId, tof_data.data()) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXclosedata(outId) != NXstatus::OKAY)
              return NXstatus::ERROR;

          }

          //---------------------------------------------------------------------------------------
          else {
            // Everything else gets copies
            if (NXgetinfo(inId, &dataRank, dataDimensions, &dataType) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXmalloc(&dataBuffer, dataRank, dataDimensions, dataType) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXgetdata(inId, dataBuffer) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXcompmakedata(outId, nxName, dataType, dataRank, dataDimensions, NX_COMP_LZW, dataDimensions) !=
                NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXopendata(outId, nxName) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (WriteAttributes(is_definition) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXputdata(outId, dataBuffer) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXfree(&dataBuffer) != NXstatus::OKAY)
              return NXstatus::ERROR;
            if (NXclosedata(outId) != NXstatus::OKAY)
              return NXstatus::ERROR;
          }

          remove_path(nxName);
        } else {
          // Make a link
          remove_path(nxName);
          strcpy(links_to_make[links_count].from, current_path);
          strcpy(links_to_make[links_count].to, link.targetPath);
          strcpy(links_to_make[links_count].name, nxName);
          links_count++;
        }
        if (NXclosedata(inId) != NXstatus::OKAY)
          return NXstatus::ERROR;
      }
    } else if (status == NXstatus::EOD) {
      if (NXclosegroup(inId) != NXstatus::OKAY)
        return NXstatus::ERROR;
      if (NXclosegroup(outId) != NXstatus::OKAY)
        return NXstatus::ERROR;
      return NXstatus::OKAY;
    }
  } while (status == NXstatus::OKAY);
  return NXstatus::OKAY;
}

//------------------------------------------------------------------------
/** Copy the attributes from input to output */
NXstatus SaveToSNSHistogramNexus::WriteAttributes(int is_definition) {
  (void)is_definition;

  NXstatus status;
  NXnumtype attrType;
  int attrLen;
  int rank;
  int dims[4];
  NXname attrName;
  void *attrBuffer;

  std::array<const char *, 6> attrs = {
      {"NeXus_version", "XML_version", "HDF_version", "HDF5_Version", "file_name", "file_time"}};

  do {
    status = NXgetnextattra(inId, attrName, &rank, dims, &attrType);
    if (status == NXstatus::ERROR)
      return NXstatus::ERROR;
    if (status == NXstatus::OKAY) {
      if (rank != 1)
        return NXstatus::ERROR;
      attrLen = dims[0];
      if (std::none_of(attrs.cbegin(), attrs.cend(),
                       [&attrName](const char *name) { return strcmp(attrName, name) == 0; })) {
        attrLen++; /* Add space for string termination */
        if (NXmalloc(&attrBuffer, 1, &attrLen, attrType) != NXstatus::OKAY)
          return NXstatus::ERROR;
        if (NXgetattr(inId, attrName, attrBuffer, &attrLen, &attrType) != NXstatus::OKAY)
          return NXstatus::ERROR;
        if (NXputattr(outId, attrName, attrBuffer, attrLen, attrType) != NXstatus::OKAY)
          return NXstatus::ERROR;
        if (NXfree(&attrBuffer) != NXstatus::OKAY)
          return NXstatus::ERROR;
      }
    }
  } while (status != NXstatus::EOD);
  return NXstatus::OKAY;
}

//------------------------------------------------------------------------
/** Execute the algorithm.
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SaveToSNSHistogramNexus::exec() {
  NXMEnableErrorReporting();

  // Retrieve the filename from the properties
  m_inputFilename = getPropertyValue("InputFileName");
  m_outputFilename = getPropertyValue("OutputFileName");
  m_compress = getProperty("Compress");

  m_inputWorkspace = getProperty("InputWorkspace");

  // We'll need to get workspace indices
  m_map = m_inputWorkspace->getDetectorIDToWorkspaceIndexMap();

  // Start the progress bar. 3 reports per histogram.
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, m_inputWorkspace->getNumberHistograms() * 3);

  EventWorkspace_const_sptr eventWorkspace = std::dynamic_pointer_cast<const EventWorkspace>(m_inputWorkspace);
  if (eventWorkspace) {
    eventWorkspace->sortAll(TOF_SORT, m_progress.get());
  }

  NXstatus ret;
  ret = this->copy_file(m_inputFilename.c_str(), NXACC_READ, m_outputFilename.c_str(), NXACC_CREATE5);

  if (ret == NXstatus::ERROR)
    throw std::runtime_error("Nexus error while copying the file.");
}

} // namespace Mantid::DataHandling
