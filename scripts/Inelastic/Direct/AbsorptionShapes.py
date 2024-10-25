# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.kernel import funcinspect
from mantid.simpleapi import (
    AbsorptionCorrection,
    ConvertUnits,
    CreateSampleWorkspace,
    CreateSampleShape,
    DeleteWorkspace,
    FlatPlateAbsorption,
    MonteCarloAbsorption,
    RenameWorkspace,
    SetSample,
    SetSampleMaterial,
    SphericalAbsorption,
)

import random
import types
import ast
import collections.abc


class anAbsorptionShape(object):
    """The parent class for all shapes, used to perform various absorption corrections
    in direct inelastic analysis.

    Contains material and environment properties necessary for absorption corrections calculations,
    as recognized by SetSampleMaterial algorithm.

    See SetSampleMaterial for full list of properties available to set.

    Usage:
    Instantiate as:
    anShape = anAbsorptionShape([ChemicalFormula,NumberDensity])
        where:
        -- ChemicalFormula is the string, describing the sample formula
        -- NumberDensity   is the number density of the sample in number
           of atoms or formula units per cubic Angstrom. If provided, will be used
           instead of the one, calculated from the Chemical formula
        e.g:
        anShape = anAbsorptionShape(['Li',0.1])
    or
    anShape = anAbsorptionShape({'ChemicalFormula':'Formula','NumberDensity':Number})
        e.g:
        anShape = anAbsorptionShape({'ChemicalFormula':'(Li7)2-C-H4-N-Cl6','NumberDensity':0.8})
    or
    anShape = anAdsrbShape(Dictionary with any number of key-value pairs recognized by SetSampleMaterial algorithm)
    e.g:
    anShape = anAbsorptionShape({'AtomicNumber':12,'AttenuationXSection':0.5,'SampleMassDensity':120})

    The Material definition should unambiguously define sample material for absorption calculations.

    The set material value may be accessed or changes through the property material: e.g.
    mat = anShape.material
    or
    anShape.material = {'ChemicalFormula':'Br'} (change existing ChemicalFormula)

    Note:
    Adding dictionary appends or modifies existing properties, but adding a list, clears up all properties, previously
    set-up on class.
    The list can contain up two members where first corresponds to the ChemicalFormula and the second one -- to the
    material's number density
    """

    # The list of the shapes defined in the module and used as the basis of the
    # absorption shapes factory
    _Defined_Shapes = {}

    def __init__(self, MaterialValue=None):
        # environment according to existing definitions of the environment
        self._Environment = {}
        # default sample material (formula)
        self._Material = {}
        # the shape holder.  Not defined in this class but the holder for all
        # other classes
        self._ShapeDescription = {}

        # the workspace used for testing correct properties settings
        rhash = random.randint(1, 100000)
        self._testWorkspace = CreateSampleWorkspace(OutputWorkspace="_adsShape_" + str(rhash), NumBanks=1, BankPixelWidth=1)

        if MaterialValue is not None:
            self.material = MaterialValue
        # If true, SetSample algorithm can set-up this shape. If not, a generic
        # CreateSampleShape algorithm should be used
        self._CanSetSample = True
        # some shapes have axis, some does not. This to distinguish between them and
        # make default axis specific to instrument
        self._shape_has_axis = False
        # Property describes if a shape axis direction have been changed.
        # Some instruments (e.g. MARI) have non-standart default axis directions
        self._axis_is_default = True

    #

    def __del__(self):
        DeleteWorkspace(self._testWorkspace)

    #

    def __str__(self):
        """Convert an absorption shape into a string representation"""
        return str(self._ShapeDescription) + "!" + str(self._Material)

    #
    @property
    def material(self):
        """Contains the material, used in absorbtion correction calculations"""
        return self._Material

    @material.setter
    def material(self, value):
        if value is None:
            self._Material = {}
            return
        if isinstance(value, str):
            value = [value]
        if isinstance(value, (list, tuple)):
            if len(value) == 2:
                self._Material = {"ChemicalFormula": value[0], "SampleNumberDensity": float(value[1])}
            elif len(value) == 1:
                self._Material = {"ChemicalFormula": value[0]}
            else:
                raise TypeError(
                    "*** If material is defined by list or tuple,"
                    " it may consist of 1 or 2 members,"
                    " defining the material formula and the material number density"
                )
        elif isinstance(value, dict):
            for key, val in value.items():
                self._Material[key] = val
        else:
            raise TypeError(
                "*** Material accepts only list or tuple containing up to 2 values"
                " corresponding to material formula and material number density"
                " or dictionary with keys, recognized by SetSampleMaterial algorithm"
            )
        #
        Mater_properties = self._Material
        if "ChemicalFormula" in Mater_properties:
            if not isinstance(Mater_properties["ChemicalFormula"], str):
                raise TypeError("*** The chemical formula for the material must be described by a string")
        # Test if the material property are recognized by SetSampleMaterial
        # algorithm.
        SetSampleMaterial(self._testWorkspace, **Mater_properties)

    #
    def correct_absorption(self, ws, *args, **kwargs):
        """The generic method, which switches between fast and Monte-Carlo absorption corrections
        depending on the type and properties variable provided as second input

        kwargs is a dictionary, with at least one key, describing type of
               corrections (fast or Monte-Carlo) and other keys (if any)
               containing additional properties of the correspondent correction
               algorithm.

        Returns:
        1) absorption-corrected workspace
        2) if two output arguments are provided, second would be workspace with absorption corrections coefficients
        """
        n_outputs, var_names = funcinspect.lhs_info("both")

        if args is None or len(args) == 0:
            if kwargs is None:
                corr_properties = {}
            else:
                corr_properties = kwargs
        else:
            corr_properties = args[0]
            if corr_properties is None:
                corr_properties = {}
            else:
                if not isinstance(corr_properties, dict):
                    raise TypeError(
                        "*** Second non-keyword argument of the correct_absorption routine"
                        " (if present) should be a dictionary containing "
                        " additional parameters for selected AbsorptionCorrections algorithm"
                    )

        correction_base_ws = ConvertUnits(ws, "Wavelength", EMode="Direct")
        Mater_properties = self._Material
        SetSampleMaterial(correction_base_ws, **Mater_properties)

        if self._shape_has_axis:
            self._check_MARI_axis_(ws)

        if self._CanSetSample:
            shape_description = self._ShapeDescription
            SetSample(correction_base_ws, Geometry=shape_description)

        mc_corrections = corr_properties.pop("is_mc", False)
        fast_corrections = corr_properties.pop("is_fast", False)
        if not (mc_corrections or fast_corrections) or (mc_corrections and fast_corrections):
            fast_corrections = False  # Case when both keys are true or false reverts to default

        if fast_corrections:
            # raise RuntimeError('Analytical absorption corrections are not currently implemented in Direct mode')
            abs_corrections = self._fast_abs_corrections(correction_base_ws, corr_properties)
        else:
            abs_corrections = self._mc_abs_corrections(correction_base_ws, corr_properties)

        abs_corrections = ConvertUnits(abs_corrections, "DeltaE", EMode="Direct")
        ws = ws / abs_corrections

        DeleteWorkspace(correction_base_ws)
        if ws.name() != var_names[0]:
            RenameWorkspace(ws, var_names[0])
        if n_outputs == 1:
            # DeleteWorkspace(abs_corrections)
            return ws
        elif n_outputs == 2:
            if abs_corrections.name() != var_names[1]:
                RenameWorkspace(abs_corrections, var_names[1])
            return (ws, abs_corrections)

    def _fast_abs_corrections(self, correction_base_ws, kwarg={}):
        """Method to correct absorption on a shape using fast (Numerical Integration) method
            if such method is available for the shape

        Not available on arbitrary shapes
        Inputs:
         ws     -- workspace to correct. Should be in the units of wavelength
        **kwarg -- dictionary of the additional keyword arguments to provide as input for
                the absorption corrections algorithm
                These arguments should not be related to the sample as the sample should already be defined
        Returns:
            workspace with absorption corrections.
        """
        adsrbtn_correctios = AbsorptionCorrection(correction_base_ws, **kwarg)
        return adsrbtn_correctios

    #
    def _mc_abs_corrections(self, correction_base_ws, kwarg={}):
        """Method to correct absorption on a shape using Mont-Carlo integration
        Inputs:
         ws     -- workspace to correct. Should be in the units of wavelength
        **kwarg -- dictionary of the additional keyword arguments to provide as input for
                the absorption corrections algorithm
                These arguments should not be related to the sample as the sample should
                already be defined.
        Returns:
            workspace with absorption corrections.
        """
        adsrbtn_correctios = MonteCarloAbsorption(correction_base_ws, **kwarg)
        return adsrbtn_correctios

    #
    @staticmethod
    def from_str(str_val):
        """Retrieve absorption shape from a string representation

        Implements shapes factory, so every new shape class should be subscribed to it
        """
        if len(anAbsorptionShape._Defined_Shapes) == 0:
            anAbsorptionShape._Defined_Shapes = {
                "Cylinder": Cylinder(),
                "FlatPlate": FlatPlate(),
                "HollowCylinder": HollowCylinder(),
                "Sphere": Sphere(),
            }

        if not isinstance(str_val, str):
            raise ValueError(
                'The input of the "from_str" function should be a string representing a diary.' " Actually it is: {0}".format(type(str_val))
            )
        str_list = str_val.split("!")
        shape_par = ast.literal_eval(str_list[0])
        mater_par = ast.literal_eval(str_list[1])

        the_shape_id = shape_par.pop("Shape", None)
        if the_shape_id is None:
            raise ValueError(
                'The input of the "from_str" function = {0} but does not contain the '
                "Shape description e.g. the Key Shape:ShapeName".format(str_val)
            )
        theShape = anAbsorptionShape._Defined_Shapes[the_shape_id]
        theShape.material = mater_par
        theShape.shape = shape_par
        return theShape

    #
    def _set_list_property(self, value, shape_name, mandatory_prop_list, opt_prop_list, opt_val_list):
        """General function to build list property for various absorption corrections algorithms
        taking it from various forms of users input and converting it into standard key-value
        dictionary.
        """
        if value is None or not value:
            self._axis_is_default = True
            return {}
        if not isinstance(value, collections.abc.Iterable):
            value = [value]
        n_elements = len(value)
        if n_elements < len(mandatory_prop_list):
            raise TypeError(
                "*** {0} shape parameter needes at least {1} imput parameters namely: {2}".format(
                    shape_name, len(mandatory_prop_list), mandatory_prop_list
                )
            )

        shape_dict = {"Shape": shape_name}
        all_prop = mandatory_prop_list + opt_prop_list
        if isinstance(value, (list, tuple)):
            for i in range(0, n_elements):
                val = value[i]
                if isinstance(val, (list, tuple)):
                    val = [float(x) for x in val]
                else:
                    val = float(val)
                shape_dict[all_prop[i]] = val
        elif isinstance(value, dict):
            for key, val in value.items():
                if isinstance(val, (list, tuple)):
                    val = [float(x) for x in val]
                else:
                    val = float(val)
                shape_dict[key] = val
        else:
            raise TypeError(
                "*** {0} shape parameter accepts only list or tuple containing from {1} to {2}"
                " values corresponding to the {0} parameters: {3} or the dictionary with these,"
                " as recognized by SetSample algorithm Geometry property".format(
                    shape_name, len(mandatory_prop_list), len(all_prop), all_prop
                )
            )
        #
        if "Axis" in shape_dict:
            self._axis_is_default = False
        for ik in range(0, len(opt_prop_list)):
            if opt_prop_list[ik] not in shape_dict:
                opt_val = opt_val_list[ik]
                if isinstance(opt_val, types.FunctionType):
                    shape_dict[opt_prop_list[ik]] = opt_val(shape_dict)
                else:
                    shape_dict[opt_prop_list[ik]] = opt_val

        return shape_dict

    #

    def _check_MARI_axis_(self, workspace):
        """method verifies, if default axis needs to be changed for MARI"""
        if self._axis_is_default:
            instrument = workspace.getInstrument()
            instr_name = instrument.getName()
            short_n = instr_name[0:3]
            if short_n.lower() == "mar":
                self._ShapeDescription["Axis"] = [1.0, 0.0, 0.0]


##---------------------------------------------------------------------------------------------------
class Cylinder(anAbsorptionShape):
    """Define the absorbing cylinder and calculate absorption corrections for this cylinder

    Usage:
    abs = Cylinder(SampleMaterial,CylinderParameters)
    The SampleMaterial is the list or dictionary as described on anAbsorptionShape class

    and

    CylinderParameters can have the form:

    a) The list consisting of 2 to 4 members.:
    CylinderParameters = [Height,Radius,[[Axis],[Center]]
    where Height, Radius are the cylinder height and radius in cm and
    Axis, if present is the direction of the cylinder wrt to the beam direction [0,0,1]
    e.g.:
    abs = Cylinder(['Al',0.1],[10,2,[1,0,0],[0,0,0]])
    b) The diary:
    CylinderParameters = {Height:Height_value,Radius:Radius_value,[Axis:axisValue],[Center:TheSampleCentre]}
    e.g:
    abs = Cylinder(['Al',0.1],[Height:10,Radius:2,Axis:[1,0,0]])

    Usage of the defined Cylinder class instance:
    Correct absorption on the cylinder using CylinderAbsorption algorithm:
    ws = abs.correct_absorption(ws) % it is default

    Correct absorption on the defined cylinder using Monte-Carlo Absorption algorithm:
    ws = ads.correct_absorption(ws,{is_mc:True,AdditionalMonte-Carlo Absorption parameters});
    """

    def __init__(self, Material=None, CylinderParams=None):
        anAbsorptionShape.__init__(self, Material)
        self.shape = CylinderParams
        self._shape_has_axis = True

    @property
    def shape(self):
        return self._ShapeDescription

    @shape.setter
    def shape(self, value):
        shape_dict = self._set_list_property(
            value, "Cylinder", ["Height", "Radius"], ["Axis", "Center"], [[0.0, 1.0, 0.0], [0.0, 0.0, 0.0]]
        )

        self._ShapeDescription = shape_dict

        if len(shape_dict) != 0:
            # Test if the shape property is recognized by CreateSampleShape
            # algorithm.
            SetSample(self._testWorkspace, Geometry=shape_dict)

    #

    def _fast_abs_corrections(self, correction_base_ws, kwarg={}):
        """Method to correct absorption on a shape using fast (Numerical Integration) method"""
        kw = kwarg.copy()
        elem_size = kw.pop("NumberOfSlices", None)
        if elem_size is not None:
            shape_dic = self.shape
            n_slices = int(shape_dic["Height"] / elem_size)
            if n_slices < 1:
                n_slices = 1
            n_annul = int(shape_dic["Radius"] * 2 * 3.1415926 / elem_size)
            if n_annul < 1:
                n_annul = 1
            kw["NumberOfSlices"] = n_slices
            kw["NumberOfAnnuli"] = n_annul
        if "Emode" not in kw:
            kw["Emode"] = "Direct"
        adsrbtn_correctios = AbsorptionCorrection(correction_base_ws, **kw)
        return adsrbtn_correctios


##---------------------------------------------------------------------------------------------------
class FlatPlate(anAbsorptionShape):
    """Define the absorbing plate and calculate absorption corrections from this plate

    Usage:
    abs = FlatPlate(SampleMaterial,PlateParameters)
    The SampleMaterial is the list or dictionary as described on anAbsorptionShape class

    and

    PlateParameters can have the form:

    a) The list consisting of 3 to 5 members e.g.:
    PlateParameters = [Height,Width,Thickness,[Centre,[Angle]]]
    where Height,Width,Thickness are the plate sizes in cm,
    the Center, if present, a list of three values indicating the [X,Y,Z] position of the center.
    he reference frame of the defined instrument is used to set the coordinate system for the shape

    The Angle argument for a flat plate shape is expected to be in degrees and is defined as the
    angle between the positive beam axis and the normal to the face perpendicular to the beam axis
    when it is not rotated, increasing in an anti-clockwise sense. The rotation is performed about
    the vertical axis of the instrument's reference frame.

    e.g.:
    abs = FlatPlate(['Al',0.1],[10,2,0.2,[1,0,0],5])
    b) The diary:
    PlateParameters = {Height:Height_value,Width:Width_value,Thickness:Thickness_value etc}
    e.g:
    abs = FlatPlate(['Al',0.1],['Height':10,'Width':4,'Thickness':2,'Angle':30)

    Usage of the defined Plate class instance:
    Correct absorption on the defined plate using AbsorptionCorrections algorithm:
    ws = abs.correct_absorption(ws,{'is_fast':True,Additiona FlatPlateAbsorption algorithm parameters})

    Correct absorption on the defined Plate using Monte-Carlo Absorption algorithm:
    ws = ads.correct_absorption(ws,{'is_mc':True,AdditionalMonte-Carlo Absorption parameters});
    """

    def __init__(self, Material=None, PlateParams=None):
        anAbsorptionShape.__init__(self, Material)
        self.shape = PlateParams

    @property
    def shape(self):
        return self._ShapeDescription

    @shape.setter
    def shape(self, value):
        shape_dict = self._set_list_property(value, "FlatPlate", ["Height", "Width", "Thick"], ["Center", "Angle"], [[0.0, 0.0, 0.0], 0.0])

        self._ShapeDescription = shape_dict

        if len(shape_dict) != 0:
            # Test if the shape property is recognized by CreateSampleShape
            # algorithm.
            SetSample(self._testWorkspace, Geometry=shape_dict)

    #
    def _fast_abs_corrections(self, correction_base_ws, kwarg={}):
        """Method to correct absorption on the FlatPlate using fast (Numerical Integration) method"""
        kw = kwarg.copy()
        prop_dict = {"Height": "SampleHeight", "Width": "SampleWidth", "Thick": "SampleThickness"}
        for key, val in prop_dict.items():
            kw[val] = self._ShapeDescription[key]
        if "Emode" not in kw:
            kw["Emode"] = "Direct"
        adsrbtn_correctios = FlatPlateAbsorption(correction_base_ws, **kw)
        return adsrbtn_correctios


##---------------------------------------------------------------------------------------------------
class HollowCylinder(anAbsorptionShape):
    """Define the Hollow absorbing cylinder and calculate absorption corrections for this cylinder

    Usage:
    abs = HollowCylinder(SampleMaterial,CylinderParameters)
    The SampleMaterial is the list or dictionary as described on anAbsorptionShape class

    and

    CylinderParameters can have the form:

    a) The list consisting of 3 to 5 members.:
    CylinderParameters = [Height,InnerRadus,OuterRadus,[[Axis],[Center]]
    where Height, InnerRadus and OuterRadus are the cylinder height and radius-es in cm and
    Axis, if present is the direction of the cylinder wrt. to the beam direction [0,0,1]
    e.g.:
    abs = HollowCylinder(['Al',0.1],[10,2,4,[0,1,0],[0,0,0]])
    b) The diary:
    CylinderParameters = {'Height':Height_value,'InnerRadus':value1,
    'OuterRadus':value2,[Axis:axisValue],[Center:TheSampleCentre]}
    e.g:
    abs = HollowCylinder(['Al',0.1],[Height:10,InnerRadus:2,OuterRadus:2,Axis:[1,0,0]])

    Usage of the defined HollowCylinder class instance:
    Correct absorption on the cylinder using CylinderAbsorption algorithm:
    ws = abs.correct_absorption(ws) % it is default

    Correct absorption on the defined cylinder using Monte-Carlo Absorption algorithm:
    ws = ads.correct_absorption(ws,{is_mc:True,AdditionalMonte-Carlo Absorption parameters});
    """

    def __init__(self, Material=None, CylinderParams=None):
        anAbsorptionShape.__init__(self, Material)
        self.shape = CylinderParams
        self._CanSetSample = False
        self._shape_has_axis = True

    #

    @property
    def shape(self):
        return self._ShapeDescription

    @shape.setter
    def shape(self, value):
        shape_dict = self._set_list_property(
            value, "HollowCylinder", ["Height", "InnerRadius", "OuterRadius"], ["Axis", "Center"], [[0.0, 1.0, 0.0], [0.0, 0.0, 0.0]]
        )
        #
        self._ShapeDescription = shape_dict
        if len(shape_dict) != 0:
            # Test if the shape property is recognized by CreateSampleShape
            # algorithm.
            self._add_xml_hollow_cylinder(self._testWorkspace)

    #
    def _add_xml_hollow_cylinder(self, ws):
        # xml shape is normaly defined in meters
        sample_xml_template = (
            """<hollow-cylinder id="HOLL_CYL">
            <centre-of-bottom-base x="{0}" y="{1}" z="{2}" />
            <axis x="{3}" y="{4}" z="{5}" />
            <inner-radius val="{6}" />
            <outer-radius val="{7}" />
            <height val="{8}" />
         </hollow-cylinder>"""
            ""
        )
        shape_dic = self._ShapeDescription
        Cenr = [c * 0.01 for c in shape_dic["Center"]]
        Axis = shape_dic["Axis"]
        sample_shape = sample_xml_template.format(
            Cenr[0],
            Cenr[1],
            Cenr[2],
            Axis[0],
            Axis[1],
            Axis[2],
            0.01 * shape_dic["InnerRadius"],
            0.01 * shape_dic["OuterRadius"],
            0.01 * shape_dic["Height"],
        )
        CreateSampleShape(ws, sample_shape)

    #
    def _fast_abs_corrections(self, correction_base_ws, kwarg={}):
        """Method to correct absorption on the HollowCylinder using fast (Numerical Integration) method"""
        self._add_xml_hollow_cylinder(correction_base_ws)

        if "Emode" not in kwarg:
            kwarg["Emode"] = "Direct"
        adsrbtn_correctios = AbsorptionCorrection(correction_base_ws, **kwarg)
        return adsrbtn_correctios

    #
    def _mc_abs_corrections(self, correction_base_ws, kwarg={}):
        """Method to correct absorption on the HollowCylinder using Monte-Carlo integration
        Inputs:
         ws     -- workspace to correct. Should be in the units of wavelength
        **kwarg -- dictionary of the additional keyword arguments to provide as input for
                the absorption corrections algorithm
                These arguments should not be related to the sample as the sample should already be defined.
        Returns:
            workspace with absorption corrections.
        """
        self._add_xml_hollow_cylinder(correction_base_ws)
        adsrbtn_correctios = MonteCarloAbsorption(correction_base_ws, **kwarg)
        return adsrbtn_correctios


##---------------------------------------------------------------------------------------------------
class Sphere(anAbsorptionShape):
    """Define the absorbing sphere and calculate absorption corrections from this sphere

    Usage:
    abs = Sphere(SampleMaterial,SphereParameters)
    The SampleMaterial is the list or dictionary as described on anAbsorptionShape class

    and

    SphereParameters can have the form:

    a) The list consisting of 1 to 2 members e.g.:
    SphereParameters = [Radius,[Center]]
    where Radius is the sphere radius in cm,
    the Center, if present, a list of three values indicating the [X,Y,Z] position of the center.
    The reference frame of the defined instrument is used to set the coordinate system
    for the shape.

    e.g.:
    abs = Sphere(['Al',0.1],[10,[1,0,0]])
    b) The diary:
    SphereParameters = {Radius:value,Centre:[1,1,1] etc}
    e.g:
    abs = Sphere(['Al',0.1],['Radius':10)

    Usage of the defined Sphere class instance:

    Correct absorption on the defined Sphere using AbsorptionCorrections algorithm:
    ws = abs.correct_absorption(ws)

    Correct absorption on the defined Sphere using Monte-Carlo Absorption algorithm:
    ws = ads.correct_absorption(ws,{'is_mc':True,AdditionalMonte-Carlo Absorption parameters});
    """

    def __init__(self, Material=None, SphereParams=None):
        anAbsorptionShape.__init__(self, Material)
        self.shape = SphereParams
        self._CanSetSample = False

    #
    @property
    def shape(self):
        return self._ShapeDescription

    #
    @shape.setter
    def shape(self, value):
        shape_dict = self._set_list_property(value, "Sphere", ["Radius"], ["Center"], [[0.0, 0.0, 0.0]])

        self._ShapeDescription = shape_dict
        if len(shape_dict) != 0:
            # Test if the shape property is recognized by CreateSampleShape
            # algorithm.
            self._add_xml_sphere(self._testWorkspace)

    def _add_xml_sphere(self, ws):
        # xml shape is normaly defined in meters
        sample_xml_template = (
            """<sphere id="WHOLE_SPHERE">
            <centre x="{0}" y="{1}" z="{2}" />
            <radius val="{3}" />
         </sphere>"""
            ""
        )
        shape_dic = self._ShapeDescription
        Cenr = [c * 0.01 for c in shape_dic["Center"]]
        sample_shape = sample_xml_template.format(Cenr[0], Cenr[1], Cenr[2], 0.01 * shape_dic["Radius"])
        CreateSampleShape(ws, sample_shape)

    #
    def _fast_abs_corrections(self, correction_base_ws, kwarg={}):
        """Method to correct absorption on the Sphere using fast (Numerical Integration) method
        (Analytical integration) method.
        If the method is invoked without parameters, optimized SphericalAbsorption algorithm is
        deployed to calculate corrections. If there are some parameters, more general
        AbsorptionCorrections method is invoked with the parameters provided.
        """
        kw = kwarg.copy()
        if "Emode" not in kwarg:
            kw["Emode"] = "Direct"
        if kw["Emode"].lower() == "elastic":
            adsrbtn_correctios = SphericalAbsorption(correction_base_ws, SphericalSampleRadius=self._ShapeDescription["Radius"])
        else:
            self._add_xml_sphere(correction_base_ws)
            adsrbtn_correctios = AbsorptionCorrection(correction_base_ws, **kw)
        return adsrbtn_correctios

    #

    def _mc_abs_corrections(self, correction_base_ws, kwarg={}):
        """Method to correct absorption on the Sphere using Monte-Carlo integration
        Inputs:
         ws     -- workspace to correct. Should be in the units of wavelength
        **kwarg -- dictionary of the additional keyword arguments to provide as input for
                the absorption corrections algorithm
                These arguments should not be related to the sample as the sample should already be defined.
        Returns:
            workspace with absorption corrections.
        """
        self._add_xml_sphere(correction_base_ws)
        adsrbtn_correctios = MonteCarloAbsorption(correction_base_ws, **kwarg)
        return adsrbtn_correctios


##---------------------------------------------------------------------------------------------------
if __name__ == "__main__":
    pass
