# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from mantid.api import AlgorithmFactory, PythonAlgorithm, FileAction, FileProperty, IMDHistoWorkspaceProperty, PropertyMode
from mantid.kernel import Direction
from lxml import etree


class SaveMDHistoToVTK(PythonAlgorithm):
    def category(self):
        return "DataHandling/XML"

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
            FileProperty("Filename", "", action=FileAction.Save, extensions=["vts"], direction=Direction.Input), doc="Output filename."
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

        B = ws.getExperimentInfo(0).sample().getOrientedLattice().getB()
        B = B / np.linalg.norm(B, axis=0)

        x = ws.getXDimension()
        X = np.linspace(x.getMinimum(), x.getMaximum(), x.getNBoundaries())
        y = ws.getYDimension()
        Y = np.linspace(y.getMinimum(), y.getMaximum(), y.getNBoundaries())
        z = ws.getZDimension()
        Z = np.linspace(z.getMinimum(), z.getMaximum(), z.getNBoundaries())

        boundingBoxInModelCoordinates = [x.getMinimum(), x.getMaximum(), y.getMinimum(), y.getMaximum(), z.getMinimum(), z.getMaximum()]
        extent = f"0 {x.getNBins()} 0 {y.getNBins()} 0 {z.getNBins()} "
        Xaxis = x.name
        Yaxis = y.name
        Zaxis = z.name

        xyz = np.meshgrid(Z, Y, X, indexing="ij")
        XYZ = np.stack((xyz[2].ravel(), xyz[1].ravel(), xyz[0].ravel()))

        changeOfBasisMatrix = np.eye(4)
        if x.getMDFrame().isQ() and y.getMDFrame().isQ() and z.getMDFrame().isQ():
            changeOfBasisMatrix[:3, :3] = B
            XYZ = np.dot(B, XYZ)

        XYZ = XYZ.flatten("F")

        root = etree.Element("VTKFile", type="StructuredGrid", version="2.2", byte_order="LittleEndian", header_type="UInt64")
        grid = etree.Element("StructuredGrid", WholeExtent=extent)

        fielddata = etree.Element("FieldData")

        change = etree.Element("DataArray", type="Float64", Name="ChangeOfBasisMatrix", NumberOfComponents="16", NumberOfTuples="1")
        change.text = " ".join(str(x) for x in changeOfBasisMatrix.flatten())
        fielddata.append(change)

        bb = etree.Element("DataArray", type="Float64", Name="BoundingBoxInModelCoordinates", NumberOfComponents="6", NumberOfTuples="1")
        bb.text = " ".join(str(x) for x in boundingBoxInModelCoordinates)
        fielddata.append(bb)

        x_axis = etree.Element("Array", type="String", Name="AxisTitleForX", ComponentName0=Xaxis, NumberOfTuples="1", format="ascii")
        x_axis.text = " ".join(str(ord(x)) for x in Xaxis) + " 0"
        fielddata.append(x_axis)

        y_axis = etree.Element("Array", type="String", Name="AxisTitleForY", ComponentName0=Yaxis, NumberOfTuples="1", format="ascii")
        y_axis.text = " ".join(str(ord(x)) for x in Yaxis) + " 0"
        fielddata.append(y_axis)

        z_axis = etree.Element("Array", type="String", Name="AxisTitleForZ", ComponentName0=Zaxis, NumberOfTuples="1", format="ascii")
        z_axis.text = " ".join(str(ord(x)) for x in Zaxis) + " 0"
        fielddata.append(z_axis)

        grid.append(fielddata)

        piece = etree.Element("Piece", Extent=extent)
        celldata = etree.Element("CellData", Scalers="Signal")

        signal_array = ws.getSignalArray()
        signal = etree.Element("DataArray", Name="Signal", type="Float64", format="ascii")
        signal.text = " ".join(f"{x:g}" for x in signal_array.ravel("F"))

        celldata.append(signal)

        mask = np.isfinite(signal_array)
        ghost_array = np.full_like(signal_array, 32, dtype=int)
        ghost_array[mask] = 0

        ghost = etree.Element("DataArray", Name="vtkGhostType", type="UInt8", format="ascii")
        ghost.text = " ".join(f"{x:g}" for x in ghost_array.ravel("F"))

        celldata.append(ghost)

        piece.append(celldata)

        points = etree.Element("Points")

        xyz = etree.Element("DataArray", type="Float64", NumberOfComponents="3", format="ascii", Name="Points")
        xyz.text = " ".join(f"{x:g}" for x in XYZ)

        points.append(xyz)
        piece.append(points)

        grid.append(piece)
        root.append(grid)

        et = etree.ElementTree(root)
        et.write(filename, pretty_print=True)


AlgorithmFactory.subscribe(SaveMDHistoToVTK)
