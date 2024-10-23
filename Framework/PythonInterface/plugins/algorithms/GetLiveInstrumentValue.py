# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, DataProcessorAlgorithm
from mantid.kernel import config, Direction, StringListValidator, StringMandatoryValidator


class GetLiveInstrumentValue(DataProcessorAlgorithm):
    def category(self):
        return "Utility"

    def summary(self):
        return "Get a live data value from and instrument via EPICS"

    def seeAlso(self):
        return ["ReflectometryReductionOneLiveData"]

    def PyInit(self):
        instruments = sorted([item.name() for item in config.getFacility().instruments()])
        instrument = config.getInstrument() if config.getInstrument() in instruments else ""
        self.declareProperty(
            name="Instrument",
            defaultValue=instrument,
            direction=Direction.Input,
            validator=StringListValidator(instruments),
            doc="Instrument to find live value for.",
        )

        self.declareProperty(
            name="PropertyType",
            defaultValue="Run",
            direction=Direction.Input,
            validator=StringListValidator(["Run", "Block"]),
            doc="The type of property to find",
        )

        self.declareProperty(
            name="PropertyName",
            defaultValue="TITLE",
            direction=Direction.Input,
            validator=StringMandatoryValidator(),
            doc="Name of value to find.",
        )

        self.declareProperty(
            name="Value",
            defaultValue="",
            direction=Direction.Output,
            doc="The live value from the instrument, or an empty string if not found",
        )

    def PyExec(self):
        self._instrument = self.getProperty("Instrument").value
        self._propertyType = self.getProperty("PropertyType").value
        self._propertyName = self.getProperty("PropertyName").value
        value = self._get_live_value()
        self._set_output_value(value)

    def _prefix(self):
        """Prefix to use at the start of the EPICS string"""
        return "IN:"

    def _name_prefix(self):
        """Prefix to use in the EPICS string before the property name"""
        if self._propertyType == "Run":
            return ":DAE:"
        else:
            return ":CS:SB:"

    @staticmethod
    def _caget(pvname, as_string=False):
        """Retrieve an EPICS PV value"""
        try:
            from CaChannel import CaChannel, CaChannelException, ca
        except ImportError:
            raise RuntimeError(
                "CaChannel must be installed to use this algorithm. Please consult the algorithm documentation for more details."
            )
        if as_string:
            dbr_type = ca.DBR_STRING
        else:
            dbr_type = None
        try:
            chan = CaChannel(pvname)
            chan.setTimeout(5.0)
            chan.searchw()
            return chan.getw(dbr_type)
        except CaChannelException as e:
            raise RuntimeError('Error reading EPICS PV "{}": {}'.format(pvname, str(e)))

    def _get_live_value(self):
        epicsName = self._prefix() + self._instrument + self._name_prefix() + self._propertyName
        return self._caget(epicsName, as_string=True)

    def _set_output_value(self, value):
        if value is not None:
            self.log().notice(self._propertyName + " = " + value)
            self.setProperty("Value", str(value))
        else:
            self.log().notice(self._propertyName + " not found")


AlgorithmFactory.subscribe(GetLiveInstrumentValue)
