# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceGroupProperty, PropertyMode, Sample
from mantid.kernel import (
    VisibleWhenProperty,
    Property,
    PropertyCriterion,
    LogicOperator,
    StringListValidator,
    IntBoundedValidator,
    FloatBoundedValidator,
    Direction,
)
from mantid.simpleapi import *


class SimpleShapeDiscusInelastic(PythonAlgorithm):
    def category(self):
        return "Workflow\\MIDAS"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("ReducedWorkspace", "", direction=Direction.Input), doc="Reduced Workspace")
        self.declareProperty(MatrixWorkspaceProperty("SqwWorkspace", "", direction=Direction.Input), doc="S(Q,w) Workspace for the sample")
        self.declareProperty(
            WorkspaceGroupProperty(
                name="OutputWorkspace", defaultValue="MuscatResults", direction=Direction.Output, optional=PropertyMode.Optional
            ),
            doc="Name for results workspaces",
        )

        self.declareProperty(name="Container", defaultValue=False, doc="Enable input of container data")

        self.declareProperty(
            name="SampleMassDensity", defaultValue=Property.EMPTY_DBL, validator=FloatBoundedValidator(0.0), doc="Sample mass density"
        )
        self.declareProperty(name="SampleChemicalFormula", defaultValue="", doc="Sample Chemical formula")

        container_condition = VisibleWhenProperty("Container", PropertyCriterion.IsEqualTo, "1")
        self.declareProperty(
            name="ContainerMassDensity",
            defaultValue=Property.EMPTY_DBL,
            validator=FloatBoundedValidator(0.0),
            doc="Container number density",
        )
        self.setPropertySettings("ContainerMassDensity", container_condition)
        self.declareProperty(name="ContainerChemicalFormula", defaultValue="V", doc="Container Chemical formula")
        self.setPropertySettings("ContainerChemicalFormula", container_condition)

        # set up shape options

        self.declareProperty(
            name="Shape",
            defaultValue="FlatPlate",
            validator=StringListValidator(["FlatPlate", "Cylinder", "Annulus"]),
            doc="Geometry of sample environment. Options are: FlatPlate, Cylinder, Annulus",
        )

        flat_plate_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "FlatPlate")
        cylinder_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "Cylinder")
        annulus_condition = VisibleWhenProperty("Shape", PropertyCriterion.IsEqualTo, "Annulus")

        container_flat_plate_condition = VisibleWhenProperty(container_condition, flat_plate_condition, LogicOperator.And)
        container_cylinder_condition = VisibleWhenProperty(container_condition, cylinder_condition, LogicOperator.And)
        container_annulus_condition = VisibleWhenProperty(container_condition, annulus_condition, LogicOperator.And)

        # height is common to all options

        self.declareProperty(
            name="Height", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Height of the sample environment (cm)"
        )

        # flat plate options

        self.declareProperty(
            name="Width", defaultValue=1.0, validator=FloatBoundedValidator(0.0), doc="Width of the FlatPlate sample environment (cm)"
        )
        self.setPropertySettings("Width", flat_plate_condition)

        self.declareProperty(
            name="Thickness",
            defaultValue=0.1,
            validator=FloatBoundedValidator(0.0),
            doc="Thickness of the FlatPlate sample environment (cm)",
        )
        self.setPropertySettings("Thickness", flat_plate_condition)

        self.declareProperty(
            name="Angle",
            defaultValue=0.0,
            validator=FloatBoundedValidator(0.0),
            doc="Angle of the FlatPlate sample environment with respect to the beam (degrees)",
        )
        self.setPropertySettings("Angle", flat_plate_condition)

        self.declareProperty(
            name="Front", defaultValue=0.1, validator=FloatBoundedValidator(0.0), doc="Thickness of the FlatPlate front (cm)"
        )
        self.setPropertySettings("Front", container_flat_plate_condition)

        self.declareProperty(
            name="Back", defaultValue=0.1, validator=FloatBoundedValidator(0.0), doc="Thickness of the FlatPlate back (cm)"
        )
        self.setPropertySettings("Back", container_flat_plate_condition)

        # cylinder options

        self.declareProperty(
            name="SampleRadius", defaultValue=0.5, validator=FloatBoundedValidator(0.0), doc="Sample radius (cm). Default=0.5"
        )
        self.setPropertySettings("SampleRadius", cylinder_condition)

        self.declareProperty(name="CanRadius", defaultValue=0.6, doc="Container outer radius (cm). Default=0.6")
        self.setPropertySettings("CanRadius", container_cylinder_condition)

        # annulus options

        self.declareProperty(
            name="SampleInnerRadius", defaultValue=0.5, validator=FloatBoundedValidator(0.0), doc="Sample inner radius (cm). Default=0.5"
        )
        self.setPropertySettings("SampleInnerRadius", annulus_condition)

        self.declareProperty(
            name="SampleOuterRadius", defaultValue=0.6, validator=FloatBoundedValidator(0.0), doc="Sample outer radius (cm). Default=0.6"
        )
        self.setPropertySettings("SampleOuterRadius", annulus_condition)

        self.declareProperty(
            name="CanInnerRadius", defaultValue=0.45, validator=FloatBoundedValidator(0.0), doc="Container inner radius (cm). Default=0.5"
        )
        self.setPropertySettings("CanInnerRadius", container_annulus_condition)

        self.declareProperty(
            name="CanOuterRadius", defaultValue=0.65, validator=FloatBoundedValidator(0.0), doc="Container outer radius (cm). Default=0.65"
        )
        self.setPropertySettings("CanOuterRadius", container_annulus_condition)

        # MC options

        self.declareProperty(
            name="NeutronPathsSingle",
            defaultValue=1000,
            validator=IntBoundedValidator(1),
            doc="Number of paths for single scattering. Default=1000",
        )
        self.declareProperty(
            name="NeutronPathsMultiple",
            defaultValue=1000,
            validator=IntBoundedValidator(1),
            doc="Number of paths for multiple scattering. Default=1000",
        )
        self.declareProperty(
            name="NumberScatterings", defaultValue=1, validator=IntBoundedValidator(1), doc="Number of scatterings. Default=1"
        )
        self.declareProperty(name="NormalizeStructureFactors", defaultValue=False, doc="Enable normalisation of structure factor")

    def PyExec(self):
        reduced_ws = self.getProperty("ReducedWorkspace").value
        sqw_ws = self.getProperty("SqwWorkspace").value

        reduced_ws.setSample(Sample())

        if self._shape == "FlatPlate":
            width = self.getProperty("Width").value
            thick = self.getProperty("Thickness").value
            angle = self.getProperty("Angle").value

            if self._can:
                front = self.getProperty("Front").value
                back = self.getProperty("Back").value

                SetSample(
                    reduced_ws,
                    Geometry={
                        "Shape": "FlatPlate",
                        "Height": self._height,
                        "Width": width,
                        "Angle": angle,
                        "Center": [0.0, 0.0, 0.0],
                        "Thick": thick,
                    },
                    Material={"ChemicalFormula": self._sam_chemical_formula, "MassDensity": self._sam_mass_density},
                    ContainerGeometry={
                        "Shape": "FlatPlateHolder",
                        "Height": self._height,
                        "Width": width,
                        "Angle": angle,
                        "Thick": thick,
                        "FrontThick": front,
                        "BackThick": back,
                        "Center": [0.0, 0.0, 0.0],
                    },
                    ContainerMaterial={"ChemicalFormula": self._can_chemical_formula, "MassDensity": self._can_mass_density},
                )

            else:
                SetSample(
                    reduced_ws,
                    Geometry={
                        "Shape": "FlatPlate",
                        "Height": self._height,
                        "Width": width,
                        "Angle": angle,
                        "Center": [0.0, 0.0, 0.0],
                        "Thick": thick,
                    },
                    Material={"ChemicalFormula": self._sam_chemical_formula, "MassDensity": self._sam_mass_density},
                )

        if self._shape == "Cylinder":
            sample_radius = self.getProperty("SampleRadius").value

            if self._can:
                container_radius = self.getProperty("CanRadius").value

                SetSample(
                    reduced_ws,
                    Geometry={"Shape": "Cylinder", "Height": self._height, "Radius": sample_radius, "Center": [0.0, 0.0, 0.0]},
                    Material={"ChemicalFormula": self._sam_chemical_formula, "MassDensity": self._sam_mass_density},
                    ContainerGeometry={
                        "Shape": "HollowCylinder",
                        "Height": self._height,
                        "InnerRadius": sample_radius,
                        "OuterRadius": container_radius,
                        "Center": [0.0, 0.0, 0.0],
                    },
                    ContainerMaterial={"ChemicalFormula": self._can_chemical_formula, "MassDensity": self._can_mass_density},
                )

            else:
                SetSample(
                    reduced_ws,
                    Geometry={"Shape": "Cylinder", "Height": self._height, "Radius": sample_radius, "Center": [0.0, 0.0, 0.0]},
                    Material={"ChemicalFormula": self._sam_chemical_formula, "MassDensity": self._sam_mass_density},
                )

        if self._shape == "Annulus":
            sample_inner = self.getProperty("SampleInnerRadius").value
            sample_outer = self.getProperty("SampleOuterRadius").value

            if self._can:
                can_inner = self.getProperty("CanInnerRadius").value
                can_inner_outer = self.getProperty("SampleInnerRadius").value
                can_outer_inner = self.getProperty("SampleOuterRadius").value
                can_outer = self.getProperty("CanOuterRadius").value

                SetSample(
                    reduced_ws,
                    Geometry={
                        "Shape": "HollowCylinder",
                        "Height": self._height,
                        "InnerRadius": sample_inner,
                        "OuterRadius": sample_outer,
                        "Center": [0.0, 0.0, 0.0],
                    },
                    Material={"ChemicalFormula": self._sam_chemical_formula, "MassDensity": self._sam_mass_density},
                    ContainerGeometry={
                        "Shape": "HollowCylinderHolder",
                        "Height": self._height,
                        "InnerRadius": can_inner,
                        "InnerOuterRadius": can_inner_outer,
                        "OuterInnerRadius": can_outer_inner,
                        "OuterRadius": can_outer,
                        "Center": [0.0, 0.0, 0.0],
                    },
                    ContainerMaterial={"ChemicalFormula": self._can_chemical_formula, "MassDensity": self._can_mass_density},
                )

            else:
                SetSample(
                    reduced_ws,
                    Geometry={
                        "Shape": "HollowCylinder",
                        "Height": self._height,
                        "InnerRadius": sample_inner,
                        "OuterRadius": sample_outer,
                        "Center": [0.0, 0.0, 0.0],
                    },
                    Material={"ChemicalFormula": self._sam_chemical_formula, "MassDensity": self._sam_mass_density},
                )

        logger.information("Geometry : %s" % self._shape)
        #        Plot3DGeometryWorkspace(Workspace=reduced_ws)

        results_group_ws = DiscusMultipleScatteringCorrection(
            InputWorkspace=reduced_ws,
            StructureFactorWorkspace=sqw_ws,
            OutputWorkspace=self._output_ws,
            NeutronPathsSingle=self._single_paths,
            NeutronPathsMultiple=self._multiple_paths,
            NumberScatterings=self._scatterings,
            NormalizeStructureFactors=self._normalise,
            startProgress=0.0,
            endProgress=1.0,
        )

        self.setProperty("OutputWorkspace", results_group_ws)

    def _setup(self):
        self._sam_chemical_formula = self.getPropertyValue("SampleChemicalFormula")
        self._sam_mass_density = self.getProperty("SampleMassDensity").value
        self._can_chemical_formula = self.getPropertyValue("ContainerChemicalFormula")
        self._can_mass_density = self.getProperty("ContainerMassDensity").value

        self._can = self.getProperty("Container").value

        # shape options
        self._shape = self.getProperty("Shape").value

        self._height = self.getProperty("Height").value

        # flat plate
        self._width = self.getProperty("Width").value
        self._thickness = self.getProperty("Thickness").value
        self._angle = self.getProperty("Angle").value

        # cylinder
        self._radius = self.getProperty("SampleRadius").value
        self._can_radius = self.getProperty("CanRadius").value

        # annulus
        self._inner_radius = self.getProperty("SampleInnerRadius").value
        self._outer_radius = self.getProperty("SampleOuterRadius").value
        self._can_inner_radius = self.getProperty("CanInnerRadius").value
        self._can_outer_radius = self.getProperty("CanOuterRadius").value

        # MC parameters
        self._single_paths = self.getProperty("NeutronPathsSingle").value
        self._multiple_paths = self.getProperty("NeutronPathsMultiple").value
        self._scatterings = self.getProperty("NumberScatterings").value
        self._normalise = self.getProperty("NormalizeStructureFactors").value

        # output
        self._output_ws = self.getPropertyValue("OutputWorkspace")

    def validateInputs(self):  # noqa: C901
        self._setup()
        issues = dict()

        if not self._sam_mass_density:
            issues["SampleMassDensity"] = "Please enter a non-zero number for sample mass density"

        if not self._sam_chemical_formula:
            issues["SampleChemicalFormula"] = "Please enter a chemical formula."

        if not self._can_mass_density:
            issues["ContainerMassDensity"] = "Please enter a non-zero number for sample mass density"

        if not self._can_chemical_formula:
            issues["ContainerChemicalFormula"] = "Please enter a chemical formula."

        if not self._height:
            issues["Height"] = "Please enter a non-zero number for height"

        if self._shape == "FlatPlate":
            if not self._width:
                issues["Width"] = "Please enter a non-zero number for width"
            if not self._thickness:
                issues["Thickness"] = "Please enter a non-zero number for thickness"

        if self._shape == "Cylinder":
            if not self._radius:
                issues["Radius"] = "Please enter a non-zero number for radius"

        if self._shape == "Annulus":
            if not self._inner_radius:
                issues["InnerRadius"] = "Please enter a non-zero number for inner radius"
            if not self._outer_radius:
                issues["OuterRadius"] = "Please enter a non-zero number for outer radius"

            # Geometry validation: outer radius > inner radius
            if not self._outer_radius > self._inner_radius:
                issues["OuterRadius"] = "Must be greater than InnerRadius"

        if self._can:
            if self._shape == "Cylinder":
                if self._container_outer_radius <= self._radius:
                    issues["CanRadius"] = "Must be greater than SampleRadius"
            if self._shape == "Annulus":
                if self._can_inner_radius >= self._inner_radius:
                    issues["CanInnerRadius"] = "Must be less than SampleInnerRadius"
                if self._can_outer_radius <= self._outer_radius:
                    issues["CanOuterRadius"] = "Must be greater than SampleOuterRadius"

        if not self._single_paths:
            issues["NeutronPathsSingle"] = "Please enter a non-zero number for neutron paths single"

        if not self._multiple_paths:
            issues["NeutronPathsMultiple"] = "Please enter a non-zero number for neutron paths multiple"

        if not self._scatterings:
            issues["NumberScatterings"] = "Please enter a non-zero number for neutron scatterings"

        return issues


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SimpleShapeDiscusInelastic)
