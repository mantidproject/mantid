# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import base64
import numpy as np
import xml.etree.ElementTree as etree
from mantid.api import AlgorithmFactory, PythonAlgorithm, FileAction, FileProperty, IMDHistoWorkspaceProperty, PropertyMode
from mantid.kernel import Direction, SpecialCoordinateSystem
from mantid.geometry import UnitCell

NUMPY_TYPE_TO_VTK = {
    np.dtype("float32"): "Float32",
    np.dtype("float64"): "Float64",
    np.dtype("int32"): "Int32",
    np.dtype("int64"): "Int64",
    np.dtype("uint8"): "UInt8",
}


class SaveMDHistoToVTK(PythonAlgorithm):
    def category(self):
        return "DataHandling\\XML"

    def summary(self):
        """
        summary of the algorithm
        :return:
        """
        return "Save a MDHistoWorkspace to a VTK file"

    def name(self):
        return "SaveMDHistoToVTK"

    def seeAlso(self):
        return ["SaveNexus"]

    def PyInit(self):
        self.declareProperty(
            IMDHistoWorkspaceProperty("InputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Input), "Input Workspace"
        )
        self.declareProperty(
            FileProperty("Filename", "", action=FileAction.Save, extensions=".vts", direction=Direction.Input), doc="Output filename."
        )

    def validateInputs(self):
        issues = dict()
        ws = self.getProperty("InputWorkspace").value
        if ws.getNumDims() != 3:
            issues["InputWorkspace"] = "Must have only 3 dimensions"

        return issues

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value
        filename = self.getPropertyValue("Filename")

        # get dimension information
        X = ws.getXDimension()
        Y = ws.getYDimension()
        Z = ws.getZDimension()

        # get skew information
        skew = self.createSkewInformation(ws)

        # create xml tree
        root = etree.Element("VTKFile", type="StructuredGrid", version="2.2", byte_order="LittleEndian", header_type="UInt64")

        extent = f"0 {X.getNBins()} 0 {Y.getNBins()} 0 {Z.getNBins()}"
        grid = etree.Element("StructuredGrid", WholeExtent=extent)

        fielddata = etree.Element("FieldData")

        # create and add change-of-basis matrix
        changeOfBasisMatrix = np.eye(4)
        changeOfBasisMatrix[:3, :3] = skew
        fielddata.append(createDataArray("ChangeOfBasisMatrix", changeOfBasisMatrix.ravel().tolist()))

        # create and add bounding box coordinates
        boundingBoxInModelCoordinates = (X.getMinimum(), X.getMaximum(), Y.getMinimum(), Y.getMaximum(), Z.getMinimum(), Z.getMaximum())
        fielddata.append(createDataArray("BoundingBoxInModelCoordinates", boundingBoxInModelCoordinates))

        # create and add axis titles
        fielddata.append(createAxis("AxisTitleForX", X.name))
        fielddata.append(createAxis("AxisTitleForY", Y.name))
        fielddata.append(createAxis("AxisTitleForZ", Z.name))

        grid.append(fielddata)

        piece = etree.Element("Piece", Extent=extent)
        celldata = etree.Element("CellData", Scalers="Signal")

        # add the signal array
        signal_array = ws.getSignalArray()
        celldata.append(createDataArrayBinary("Signal", signal_array.ravel("F")))

        # create and add a mask array
        mask = np.isfinite(signal_array)
        ghost_array = np.full_like(signal_array, 32, dtype=np.uint8)  # 32 == CellGhostTypes::HIDDENCELL
        ghost_array[mask] = 0
        celldata.append(createDataArrayBinary("vtkGhostType", ghost_array.ravel("F")))

        piece.append(celldata)

        # create and add cell boundary points
        points = etree.Element("Points")
        XYZ = createPointsArray(ws, skew)
        points.append(createDataArrayBinary("Points", XYZ.ravel("F"), 3))
        piece.append(points)

        grid.append(piece)
        root.append(grid)

        et = etree.ElementTree(root)
        et.write(filename)

    def createSkewInformation(self, ws):
        # logic here is based on vtkDataSetToNonOrthogonalDataSet::createSkewInformation from mantid v5.1.1
        if (
            ws.getSpecialCoordinateSystem() == SpecialCoordinateSystem.HKL
            and ws.getXDimension().getMDFrame().isQ()
            and ws.getYDimension().getMDFrame().isQ()
            and ws.getZDimension().getMDFrame().isQ()
            and ws.getNumExperimentInfo() > 0
            and ws.getExperimentInfo(0).sample().hasOrientedLattice()
        ):
            self.log().information("Creating non-orthogonal skew matrix")

            skew = B = ws.getExperimentInfo(0).sample().getOrientedLattice().getB()

            run = ws.getExperimentInfo(0).run()
            if run.hasProperty("W_MATRIX"):
                self.log().information("Using W_MATRIX to calculate skew matrix")
                W = np.array(run.getProperty("W_MATRIX").value).reshape((3, 3))
                B = B.dot(W)
                uc = UnitCell()
                uc.recalculateFromGstar(B.T.dot(B))
                skew = uc.getB()
            else:
                self.log().information("Using basis vectors to calculate skew matrix")
                basis = np.eye(3)

                try:
                    basis[0] = list(ws.getBasisVector(0))
                    basis[1] = list(ws.getBasisVector(1))
                    basis[2] = list(ws.getBasisVector(2))
                except ValueError:
                    self.log().warning("Invalid basis vector found")
                else:
                    if np.linalg.norm(basis) != 0:
                        skew = basis.dot(B.dot(basis.T))

            skew = skew / np.linalg.norm(skew, axis=0)

            return skew

        return np.eye(3)


def createPointsArray(ws, skew):
    x = ws.getXDimension()
    X = np.linspace(x.getMinimum(), x.getMaximum(), x.getNBoundaries())
    y = ws.getYDimension()
    Y = np.linspace(y.getMinimum(), y.getMaximum(), y.getNBoundaries())
    z = ws.getZDimension()
    Z = np.linspace(z.getMinimum(), z.getMaximum(), z.getNBoundaries())

    xyz = np.meshgrid(Z, Y, X, indexing="ij")
    XYZ = np.stack((xyz[2].ravel(), xyz[1].ravel(), xyz[0].ravel()))

    return skew.dot(XYZ)


def createDataArray(name, data):
    dataArray = etree.Element("DataArray", type="Float64", Name=name, NumberOfComponents=str(len(data)), NumberOfTuples="1")
    dataArray.text = " ".join(f"{x:.16g}" for x in data)
    return dataArray


def createAxis(name, title):
    axis = etree.Element("Array", type="String", Name=name, ComponentName0=title, NumberOfTuples="1", format="ascii")
    axis.text = " ".join(str(ord(x)) for x in title) + " 0"  # NULL terminated
    return axis


def createDataArrayBinary(name, data, number_of_componets=1):
    dataArray = etree.Element(
        "DataArray",
        Name=name,
        type=NUMPY_TYPE_TO_VTK[data.dtype],
        format="binary",
        RangeMin=str(np.nanmin(data)),
        RangeMax=str(np.nanmax(data)),
        NumberOfComponents=str(number_of_componets),
    )
    dataArray.text = base64.b64encode(np.uint64(data.nbytes).tobytes() + data.tobytes()).decode()
    return dataArray


AlgorithmFactory.subscribe(SaveMDHistoToVTK)
