#pylint: disable=no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, \
    InstrumentValidator, FileProperty, FileAction
from mantid.kernel import Direction, StringArrayProperty

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
        <rot axis-x="0" axis-y="1" axis-z="0" val="%(Y1)f">
          <rot axis-x="0" axis-y="0" axis-z="1" val="%(Z)f">
            <rot axis-x="0" axis-y="1" axis-z="0" val="%(Y2)f"/>
          </rot>
        </rot>
      </location>

"""

# no rotation needs to be specified
COMPONENT_XML_MINIMAL = """      <location x="%(x)f" y="%(y)f" z="%(z)f" name="%(name)s">
      </location>

"""


class ExportGeometry(PythonAlgorithm):
    def category(self):
        return "Utility"

    def name(self):
        return "ExportGeometry"

    def summary(self):
        return "Extract geometry into a variety of file formats"

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("InputWorkspace", "",
                                               validator=InstrumentValidator(),
                                               direction=Direction.Input),
                             doc="Workspace containing the instrument to be exported")
        self.declareProperty(StringArrayProperty("Components",
                                                 direction=Direction.Input),
                             doc="Comma separated list of instrument components to export")
        self.declareProperty(FileProperty(name="Filename",
                                          defaultValue="",
                                          action=FileAction.Save,
                                          extensions=[".xml"]),
                             doc="Save file")

    def validateInputs(self):
        issues = {}

        # get the input workspace
        wksp = self.getProperty("InputWorkspace").value

        # confirm that all of the requested components exist
        components = self.getProperty("Components").value
        if len(components) <= 0:
            issues['Components'] = "Must supply components"
        else:
            components = [component for component in components
                          if wksp.getInstrument().getComponentByName(component) is None]
            if len(components) > 0:
                issues['Components'] = "Instrument has no component \"" \
                                       + ','.join(components) + "\""

        return issues

    def __updatePos(self, info, component):
        pos = component.getPos()
        info['x'] = pos.X()
        info['y'] = pos.Y()
        info['z'] = pos.Z()

        angles = component.getRotation().getEulerAngles('YZY')
        info['Y1'] = angles[0]
        info['Z'] = angles[1]
        info['Y2'] = angles[2]

    def __writexmlSource(self, handle, instrument):
        source = {}
        self.__updatePos(source, instrument.getSource())
        handle.write(SOURCE_XML % source)

        sample = {}
        self.__updatePos(sample, instrument.getSample())
        handle.write(SAMPLE_XML % sample)

    def __writexml(self, handle, component):
        info = {'name': component.getName()}
        self.__updatePos(info, component)

        if info['Y1'] == 0. and info['Z'] == 0. and info['Y2'] == 0.:
            handle.write(COMPONENT_XML_MINIMAL % info)
        else:
            handle.write(COMPONENT_XML_FULL % info)

    def PyExec(self):
        wksp = self.getProperty("InputWorkspace").value
        components = self.getProperty("Components").value
        filename = self.getProperty("Filename").value

        instrument = wksp.getInstrument()
        with open(filename, 'w') as handle:
            # write out the source and sample components
            self.__writexmlSource(handle, instrument)

            # write out the requested components
            handle.write("""  <!--COMPONENTS-->\n""")
            for component in components:
                component = instrument.getComponentByName(component)
                self.__writexml(handle, component)

AlgorithmFactory.subscribe(ExportGeometry)
