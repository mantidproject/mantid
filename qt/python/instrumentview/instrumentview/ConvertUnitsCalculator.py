from mantid.dataobjects import Workspace2D
from mantid.kernel import UnitConversion, DeltaEModeType, UnitParametersMap, UnitParams


class ConvertUnitsCalculator:
    def __init__(self, ws: Workspace2D) -> None:
        self._detector_info = ws.detectorInfo()
        self._l1 = self._detector_info.l1()

    def convert(self, source_unit: str, target_unit: str, detector_id: int, value: float) -> float:
        if source_unit == target_unit:
            return value
        detector_index = self._detector_info.indexOf(int(detector_id))
        parameter_map = UnitParametersMap()
        parameter_map[UnitParams.l2] = self._detector_info.l2(detector_index)
        parameter_map[UnitParams.twoTheta] = self._detector_info.twoTheta(detector_index)
        return UnitConversion.run(source_unit, target_unit, value, self._l1, DeltaEModeType.Elastic, parameter_map)
