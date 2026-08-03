# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, InstrumentValidator, FileProperty, FileAction
from mantid.kernel import Direction, StringArrayProperty, StringListValidator
from plugins.algorithms.component_info_utils import resolve_component_index

SOURCE_XML = """  <!--SOURCE-->
  <component type="moderator">
    <location z="%(z)f"/>
  </component>
  <type is="Source" name="moderator"/>

"""

SAMPLE_XML = """  <!--SAMPLE-->
  <component type="sample-position">
    <location x="%(x)f" y="%(y)f" z="%(z)f"/>
  </component>
  <type is="SamplePos" name="sample-position"/>

"""

COMPONENT_XML_FULL = """      <location x="%(x)f" y="%(y)f" z="%(z)f" name="%(name)s">
        <rot %(alpha_string)s">
          <rot %(beta_string)s">
            <rot %(gamma_string)s"/>
          </rot>
        </rot>
      </location>

"""

# no rotation needs to be specified
COMPONENT_XML_MINIMAL = """      <location x="%(x)f" y="%(y)f" z="%(z)f" name="%(name)s">
      </location>

"""


class ExportGeometry(PythonAlgorithm):
    _eulerCon = None
    _eulerXML = {
        "X": 'axis-x="1" axis-y="0" axis-z="0" val="',
        "Y": 'axis-x="0" axis-y="1" axis-z="0" val="',
        "Z": 'axis-x="0" axis-y="0" axis-z="1" val="',
    }

    def category(self):
        return "Utility\\Instrument"

    def seeAlso(self):
        return ["LoadInstrument"]

    def name(self):
        return "ExportGeometry"

    def summary(self):
        return "Extract components from larger in-memory instrument, save as IDF style xml"

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", validator=InstrumentValidator(), direction=Direction.Input),
            doc="Workspace containing the instrument to be exported",
        )
        eulerConventions = ["ZXZ", "XYX", "YZY", "ZYZ", "XZX", "YXY", "XYZ", "YZX", "ZXY", "XZY", "ZYX", "YXZ"]
        self.declareProperty(
            name="EulerConvention",
            defaultValue="YZY",
            validator=StringListValidator(eulerConventions),
            doc="Euler angles convention used when writing angles.",
        )
        self.declareProperty(
            StringArrayProperty("Components", direction=Direction.Input), doc="Comma separated list of instrument component names to export"
        )
        self.declareProperty(FileProperty(name="Filename", defaultValue="", action=FileAction.Save, extensions=[".xml"]), doc="Save file")

    def validateInputs(self):
        issues = {}

        # get the input workspace
        wksp = self.getProperty("InputWorkspace").value

        # confirm that all of the requested components exist
        components = self.getProperty("Components").value
        if len(components) <= 0:
            issues["Components"] = "Must supply components"
        else:
            component_info = wksp.componentInfo()

            def component_missing(name):
                try:
                    resolve_component_index(name, component_info)
                    return False
                except ValueError:
                    return True

            components = [component for component in components if component_missing(component)]
            if len(components) > 0:
                issues["Components"] = 'Instrument has no component "' + ",".join(components) + '"'

        return issues

    def __updatePos(self, info, component_info, index):
        pos = component_info.position(index)
        info["x"] = pos.X()
        info["y"] = pos.Y()
        info["z"] = pos.Z()

        angles = component_info.rotation(index).getEulerAngles(self._eulerCon)
        info["alpha"] = angles[0]
        info["beta"] = angles[1]
        info["gamma"] = angles[2]
        info["alpha_string"] = self._eulerXML[self._eulerCon[0]] + str(angles[0])
        info["beta_string"] = self._eulerXML[self._eulerCon[1]] + str(angles[1])
        info["gamma_string"] = self._eulerXML[self._eulerCon[2]] + str(angles[2])

    def __writexmlSource(self, handle, component_info):
        source = {}
        self.__updatePos(source, component_info, component_info.source())
        handle.write(SOURCE_XML % source)

        sample = {}
        self.__updatePos(sample, component_info, component_info.sample())
        handle.write(SAMPLE_XML % sample)

    def __writexml(self, handle, component_info, index):
        info = {"name": component_info.name(index)}
        self.__updatePos(info, component_info, index)

        if info["alpha"] == 0.0 and info["beta"] == 0.0 and info["gamma"] == 0.0:
            handle.write(COMPONENT_XML_MINIMAL % info)
        else:
            handle.write(COMPONENT_XML_FULL % info)

    def PyExec(self):
        wksp = self.getProperty("InputWorkspace").value
        components = self.getProperty("Components").value
        filename = self.getProperty("Filename").value
        self._eulerCon = self.getProperty("EulerConvention").value

        component_info = wksp.componentInfo()
        with open(filename, "w") as handle:
            # write out the source and sample components
            self.__writexmlSource(handle, component_info)

            # write out the requested components
            handle.write("""  <!--COMPONENTS-->\n""")
            for component in components:
                index = resolve_component_index(component, component_info)
                self.__writexml(handle, component_info, index)


AlgorithmFactory.subscribe(ExportGeometry)
