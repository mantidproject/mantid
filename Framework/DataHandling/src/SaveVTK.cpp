// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveVTK.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"
#include <Poco/File.h>
#include <fstream>
#include <string>

namespace Mantid::DataHandling {

// Register algorithm with AlgorithmFactory
DECLARE_ALGORITHM(SaveVTK)

using namespace Kernel;
using namespace DataObjects;
using namespace API;

/// Default constructor
SaveVTK::SaveVTK() : m_Xmin(0), m_Xmax(0) {}

/**
 * Initilisation method.
 * Simply declares the properties that this algorithm possesses
 */
void SaveVTK::init() {
  // Declare mandatory properties
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The workspace name to use as input");
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Save),
                  "The name to use when writing the file");
  // Declare optional properties
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("Xminimum", 0.0, mustBePositive,
                  "No bin that contains x values lower than this will be saved (default\n"
                  "0)");
  declareProperty("Xmaximum", 0.0, mustBePositive,
                  "No bin that contains x values higher than this will saved (default\n"
                  "0 signifies the highest x value)");
}

/**
 * Executes the algorithm.
 * Saves the workspace specified by the user to the VTK XML format
 */
void SaveVTK::exec() {
  std::string filename = getProperty("Filename");
  g_log.debug() << "Parameters: Filename='" << filename << "'\n";
  // add extension
  filename += ".vtu";

  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  if (!inputWorkspace) {
    g_log.error("Failed to retrieve inputWorkspace.");
    throw Exception::NullPointerException("SaveVTK::exec()", "inputWorkspace");
  }

  checkOptionalProperties();

  // Open file for writing
  std::ofstream outVTP(filename.c_str());
  if (!outVTP) {
    g_log.error("Failed to open file: " + filename);
    throw Exception::FileError("Failed to open file ", filename);
  }

  // First write document level XML header
  outVTP << "<?xml version=\"1.0\"?>\n"
            "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" "
            "byte_order=\"LittleEndian\">\n"
            "<UnstructuredGrid>\n";

  const std::string workspaceID = inputWorkspace->id();
  if (workspaceID.find("Workspace2D") != std::string::npos) {
    const Workspace2D_sptr localWorkspace = std::dynamic_pointer_cast<Workspace2D>(inputWorkspace);

    // Write out whole range
    bool xMin(m_Xmin > 0.0), xMax(m_Xmax > 0.0);
    Progress prog(this, 0.0, 1.0, 97);
    if (!xMin && !xMax) {
      for (int hNum = 2; hNum < 100; ++hNum) {
        writeVTKPiece(outVTP, localWorkspace->x(hNum).rawData(), localWorkspace->y(hNum).rawData(),
                      localWorkspace->e(hNum).rawData(), hNum);
        prog.report();
      }
    } else {
      for (int hNum = 2; hNum < 100; ++hNum) {

        auto &X = localWorkspace->x(hNum);
        auto &Y = localWorkspace->y(hNum);
        auto &E = localWorkspace->e(hNum);

        std::vector<double> xValue, yValue, errors;
        std::vector<double>::size_type nVals(Y.size());

        for (int i = 0; i < static_cast<int>(nVals); ++i) {

          if (xMin && X[i] < m_Xmin) {
            continue;
          }

          if (xMax && X[i + 1] > m_Xmax) {
            xValue.emplace_back(X[i]);
            break;
          }

          xValue.emplace_back(X[i]);
          if (i == static_cast<int>(nVals) - 1) {
            xValue.emplace_back(X[i + 1]);
          }

          yValue.emplace_back(Y[i]);
          errors.emplace_back(E[i]);
        }
        // sanity check
        assert((int)xValue.size() == (int)yValue.size() + 1);

        writeVTKPiece(outVTP, xValue, yValue, errors, hNum);
        prog.report();
      }
    }
  } else {
    outVTP.close();
    Poco::File(filename).remove();
    throw Exception::NotImplementedError("SaveVTK only implemented for Workspace2D\n");
  }

  // Final XML end block tags
  outVTP << "</UnstructuredGrid>\n</VTKFile>\n";
  outVTP.close();
}

/**
 * Check the validity of the optional parameters
 */
void SaveVTK::checkOptionalProperties() {
  m_Xmin = getProperty("Xminimum");
  m_Xmax = getProperty("Xmaximum");

  if (m_Xmin > 0. && m_Xmax > 0 && m_Xmin > m_Xmax) {
    g_log.error("Invalid range specification.");
    throw std::invalid_argument("SaveVTK: Inconsistent range values");
  }
}

/**
 * Write a histogram as a VTK <Piece .../> block
 * @param outVTP :: The output stream
 * @param xValue :: The x data
 * @param yValue :: The y data
 * @param errors :: The error data
 * @param index :: The histogram number
 */
void SaveVTK::writeVTKPiece(std::ostream &outVTP, const std::vector<double> &xValue, const std::vector<double> &yValue,
                            const std::vector<double> &errors, int index) const {
  (void)errors; // Avoid compiler warning

  auto nY = static_cast<int>(yValue.size());
  int nPoints(8 * nY);
  outVTP << "<Piece NumberOfPoints=\"" << nPoints << "\" NumberOfCells=\"" << nY << "\">";
  outVTP << "<CellData Scalars=\"counts\">"
         << "<DataArray type=\"Float32\" Name=\"counts\" "
            "NumberOfComponents=\"1\" format=\"ascii\">\n";
  for (int i = 0; i < nY; ++i) {
    outVTP << yValue[i] << "\n";
  }
  outVTP << "</DataArray></CellData>\n";

  outVTP << "<Points>"
         << "<DataArray type=\"Float32\" NumberOfComponents=\"3\" "
            "format=\"ascii\">\n";

  double deltaZ(100.);
  for (int i = 0; i < nY; ++i) {
    // first face
    double xLow(xValue[i]), xUpp(xValue[i + 1]), ypos(yValue[i]), zpos(-index * deltaZ);
    outVTP << xLow << " " << 0.0 << " " << zpos << "\n";
    outVTP << xUpp << " " << 0.0 << " " << zpos << "\n";
    outVTP << xLow << " " << ypos << " " << zpos << "\n";
    outVTP << xUpp << " " << ypos << " " << zpos << "\n";
    // second face
    zpos = -(index + 1) * deltaZ;
    outVTP << xLow << " " << 0.0 << " " << zpos << "\n";
    outVTP << xUpp << " " << 0.0 << " " << zpos << "\n";
    outVTP << xLow << " " << ypos << " " << zpos << "\n";
    outVTP << xUpp << " " << ypos << " " << zpos << "\n";
  }
  outVTP << "</DataArray></Points>\n";
  outVTP << "<Cells>\n"
         << "<DataArray type=\"Int32\" Name =\"connectivity\" format=\"ascii\">\n";
  for (int i = 0; i < nPoints; ++i) {
    outVTP << i << "\n";
  }
  outVTP << "</DataArray>\n";
  outVTP << "<DataArray type=\"Int32\" Name =\"offsets\" format=\"ascii\">\n";
  for (int i = 8; i <= nPoints; i += 8) {
    outVTP << i << "\n";
  }
  outVTP << "</DataArray>\n";
  outVTP << "<DataArray type=\"UInt8\" Name =\"types\" format=\"ascii\">\n";
  for (int i = 0; i < nPoints; ++i) {
    outVTP << "11\n";
  }
  outVTP << "</DataArray>\n";

  outVTP << "</Cells>\n";
  // End of this piece
  outVTP << "</Piece>\n";
}
} // namespace Mantid::DataHandling
