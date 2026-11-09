from __future__ import annotations

import contextlib
import io
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).parents[1]))
import generate_simpleapi_stubs as generator


def _property(
    name: str,
    type_name: str,
    *,
    valid: str = "",
    allowed: tuple[str, ...] = (),
    class_name: str = "Property",
    optional: bool = False,
):
    prop = type(class_name, (), {})()
    prop.name = name
    prop.type = type_name
    prop.isValid = valid
    prop.allowedValues = list(allowed)
    prop.isOptional = lambda: optional
    return prop


class _ConvertUnits:
    def __init__(self):
        self.properties = {
            "InputWorkspace": _property("InputWorkspace", "MatrixWorkspace", valid="Required", class_name="MatrixWorkspaceProperty"),
            "Target": _property("Target", "string", valid="Required", allowed=("DeltaE", "Wavelength")),
            "EMode": _property("EMode", "string", allowed=("Elastic", "Direct", "Indirect")),
            "OutputWorkspace": _property("OutputWorkspace", "MatrixWorkspace", class_name="MatrixWorkspaceProperty"),
        }

    def initialize(self):
        pass

    def mandatoryProperties(self):
        return ["InputWorkspace", "Target", "EMode"]

    def outputProperties(self):
        return ["OutputWorkspace"]

    def getProperty(self, name: str):
        return self.properties[name]


class _FittingAlgorithm:
    def __init__(self, properties, outputs, inouts=(), inputs=()):
        self.properties = properties
        self.outputs = outputs
        self.inouts = inouts
        self.inputs = inputs

    def initialize(self):
        pass

    def mandatoryProperties(self):
        return ["Function", "InputWorkspace", *self.inputs]

    def outputProperties(self):
        return self.outputs

    def inoutProperties(self):
        return self.inouts

    def getProperty(self, name: str):
        return self.properties[name]


class _PropertyAlgorithm:
    def __init__(self, properties, inputs, outputs):
        self.properties = properties
        self.inputs = inputs
        self.outputs = outputs

    def initialize(self):
        pass

    def mandatoryProperties(self):
        return self.inputs

    def outputProperties(self):
        return self.outputs

    def getProperty(self, name: str):
        return self.properties[name]


def _rebin_algorithm():
    return _PropertyAlgorithm(
        {
            "InputWorkspace": _property("InputWorkspace", "MatrixWorkspace", valid="Required", class_name="MatrixWorkspaceProperty"),
            "Params": _property("Params", "dbl list", valid="Required", class_name="FloatArrayProperty"),
            "PreserveEvents": _property("PreserveEvents", "boolean", class_name="BoolPropertyWithValue"),
            "Power": _property("Power", "number", class_name="FloatPropertyWithValue"),
            "BinningMode": _property("BinningMode", "string", allowed=("Default", "Linear")),
            "OutputWorkspace": _property("OutputWorkspace", "MatrixWorkspace", class_name="MatrixWorkspaceProperty"),
        },
        inputs=["InputWorkspace", "Params", "PreserveEvents", "Power", "BinningMode"],
        outputs=["OutputWorkspace"],
    )


def _mantid_modules():
    mantid = types.ModuleType("mantid")
    mantid.__path__ = []
    simpleapi = types.ModuleType("mantid.simpleapi")
    simpleapi.__SPECIALIZED_FUNCTIONS__ = []
    api = types.ModuleType("mantid.api")
    api.AlgorithmFactory = types.SimpleNamespace(getRegisteredAlgorithms=lambda _include_hidden: {"ConvertUnits": [1]})
    api.AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: _ConvertUnits())
    api.MatrixWorkspace = type("MatrixWorkspace", (), {})
    api.Workspace = type("Workspace", (), {})
    dataobjects = types.ModuleType("mantid.dataobjects")
    mantid.simpleapi = simpleapi
    mantid.api = api
    mantid.dataobjects = dataobjects
    return {
        "mantid": mantid,
        "mantid.simpleapi": simpleapi,
        "mantid.api": api,
        "mantid.dataobjects": dataobjects,
    }


class GenerateSimpleAPIStubsTest(unittest.TestCase):
    def setUp(self):
        generator._workspace_imports.clear()
        generator._workspace_import_names.clear()
        generator._used_workspace_types.clear()
        generator._unsupported_property_types.clear()
        generator._used_function_types = False
        generator._used_array_aliases.clear()
        generator._workspace_imports["MatrixWorkspace"] = "mantid.api"
        generator._workspace_import_names["MatrixWorkspace"] = "MatrixWorkspace"

    def test_convert_units_signature_uses_property_metadata(self):
        with patch.dict(sys.modules, _mantid_modules()):
            _, stub = generator._stub_for_algorithm("ConvertUnits", 1)

        self.assertIn("InputWorkspace: MatrixWorkspace | str", stub)
        self.assertIn("Target: Literal['DeltaE', 'Wavelength']", stub)
        self.assertIn("EMode: Literal['Elastic', 'Direct', 'Indirect'] | None = ...", stub)
        self.assertIn("OutputWorkspace: MatrixWorkspace | str | None = ...", stub)
        self.assertIn(") -> MatrixWorkspace: ...", stub)
        self.assertNotIn("Target: Literal['DeltaE', 'Wavelength'] =", stub)

    def test_rebin_parameters_accept_strings_and_arrays(self):
        algorithm = _rebin_algorithm()
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)

        with patch.dict(sys.modules, modules):
            _, stub = generator._stub_for_algorithm("Rebin", 1)

        self.assertIn("Params: _Float1DArray | float | str,", stub)
        self.assertIn("PreserveEvents: bool | str | None = ...", stub)
        self.assertIn("Power: float | str | None = ...", stub)
        self.assertIn("BinningMode: Literal['Default', 'Linear'] | None = ...", stub)
        self.assertIn("InputWorkspace: MatrixWorkspace | str,", stub)
        self.assertTrue(stub.endswith("-> MatrixWorkspace: ..."))
        self.assertEqual(generator._used_array_aliases, {"_Float1DArray"})

    def test_file_property_extensions_are_not_emitted_as_literals(self):
        filename = _property("Filename", "string", valid="Required", allowed=(".nxs.h5", ".nxs", "_event.nxs"), class_name="FileProperty")
        algorithm = _PropertyAlgorithm(
            {
                "Filename": filename,
                "OutputWorkspace": _property("OutputWorkspace", "MatrixWorkspace", class_name="MatrixWorkspaceProperty"),
            },
            inputs=["Filename"],
            outputs=["OutputWorkspace"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)

        with patch.dict(sys.modules, modules):
            _, stub = generator._stub_for_algorithm("LoadEventNexus", 1)

        self.assertIn("Filename: str,", stub)
        self.assertNotIn(".nxs", stub)

    def test_array_aliases_are_declared_only_when_used(self):
        def generate(name, algorithm):
            modules = _mantid_modules()
            modules["mantid.api"].AlgorithmFactory = types.SimpleNamespace(getRegisteredAlgorithms=lambda _include_hidden: {name: [1]})
            modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)
            with tempfile.TemporaryDirectory() as temporary_directory:
                output = Path(temporary_directory) / "simpleapi.pyi"
                argv = ["generate_simpleapi_stubs.py", name, "--output", str(output)]
                with (
                    patch.dict(sys.modules, modules),
                    patch.object(generator, "_format_with_ruff", side_effect=lambda contents: contents),
                    patch.object(sys, "argv", argv),
                ):
                    self.assertEqual(generator.main(), 0)
                return output.read_text()

        rebin = generate("Rebin", _rebin_algorithm())
        self.assertIn("from collections.abc import Sequence\n", rebin)
        self.assertIn("import numpy\n", rebin)
        self.assertIn("type _Float1DArray = Sequence[float] | numpy.ndarray\n", rebin)
        self.assertNotIn("_Int1DArray =", rebin)

        self.setUp()
        convert_units = generate("ConvertUnits", _ConvertUnits())
        self.assertNotIn("Sequence", convert_units)
        self.assertNotIn("numpy", convert_units)
        self.assertNotIn("1DArray", convert_units)

    def test_parameter_types_widen_to_strings_without_duplicates(self):
        cases = {
            ("dbl list", "FloatArrayProperty"): "_Float1DArray | float | str",
            ("int list", "ArrayProperty"): "_Int1DArray | int | str",
            ("unsigned int list", "ArrayProperty"): "_Int1DArray | int | str",
            ("str list", "ArrayProperty"): "_Str1DArray | str",
            ("list of str lists", "ArrayProperty"): "Sequence[_Str1DArray] | str",
            ("list of str lists", "MultipleFileProperty"): "str | list[str]",
            ("number", "IntPropertyWithValue"): "int | str",
            ("optional boolean", "PropertyWithValue"): "bool | str | None",
            ("Dictionary", "PropertyManagerProperty"): "dict[str, Any] | str",
            ("string", "PropertyWithValue"): "str",
            ("Function", "FunctionProperty"): "str | IFunction",
        }
        for (type_name, class_name), expected in cases.items():
            with self.subTest(type_name=type_name):
                self.assertEqual(generator._parameter_type_for(_property("Prop", type_name, class_name=class_name)), expected)

    def test_function_properties_use_ifunction_and_wrapper_return(self):
        algorithm = _PropertyAlgorithm(
            {
                "Function": _property("Function", "Function", valid="Required", class_name="FunctionProperty"),
                "OutputFunction": _property("OutputFunction", "Function", class_name="FunctionProperty"),
            },
            inputs=["Function"],
            outputs=["OutputFunction"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)

        with patch.dict(sys.modules, modules):
            _, stub = generator._stub_for_algorithm("ConvertFitFunction", 1)

        self.assertIn("Function: str | IFunction", stub)
        self.assertIn("OutputFunction: str | IFunction | None = ...", stub)
        self.assertTrue(stub.endswith("-> FunctionWrapper: ..."))
        self.assertTrue(generator._used_function_types)
        self.assertNotIn("Function", generator._unsupported_property_types)

    def test_ordinary_multiple_outputs_use_namedtuple_variants(self):
        algorithm = _PropertyAlgorithm(
            {
                "OutputWorkspace": _property("OutputWorkspace", "Workspace", class_name="WorkspaceProperty"),
                "OutputParameterWorkspace": _property(
                    "OutputParameterWorkspace", "TableWorkspace", class_name="ITableWorkspaceProperty", optional=True
                ),
                "OutputMessage": _property("OutputMessage", "string"),
            },
            inputs=[],
            outputs=["OutputWorkspace", "OutputParameterWorkspace", "OutputMessage"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)
        generator._workspace_imports.update({"Workspace": "mantid.api", "TableWorkspace": "mantid.api"})
        generator._workspace_import_names.update({"Workspace": "Workspace", "TableWorkspace": "TableWorkspace"})

        with patch.dict(sys.modules, modules):
            records, stub = generator._stub_for_algorithm("ExampleMultiOutput", 1)

        self.assertIn("-> _ExampleMultiOutputReturns1 | _ExampleMultiOutputReturns2", stub)
        self.assertIn("OutputParameterWorkspace: TableWorkspace", "\n".join(records))
        self.assertIn("OutputMessage: str", "\n".join(records))
        self.assertIn("OutputWorkspace: Workspace", "\n".join(records))

    def test_optional_single_workspace_output_includes_none(self):
        algorithm = _PropertyAlgorithm(
            {"OutputWorkspace": _property("OutputWorkspace", "Workspace", class_name="WorkspaceProperty", optional=True)},
            inputs=[],
            outputs=["OutputWorkspace"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)
        generator._workspace_imports["Workspace"] = "mantid.api"
        generator._workspace_import_names["Workspace"] = "Workspace"

        with patch.dict(sys.modules, modules):
            records, stub = generator._stub_for_algorithm("OptionalOutput", 1)

        self.assertEqual(records, [])
        self.assertTrue(stub.endswith("-> None | Workspace: ..."))

    def test_many_optional_workspace_outputs_use_conservative_return(self):
        output_names = [f"OutputWorkspace{index}" for index in range(4)]
        algorithm = _PropertyAlgorithm(
            {name: _property(name, "Workspace", class_name="WorkspaceProperty", optional=True) for name in output_names},
            inputs=[],
            outputs=output_names,
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)
        generator._workspace_imports["Workspace"] = "mantid.api"
        generator._workspace_import_names["Workspace"] = "Workspace"

        with patch.dict(sys.modules, modules):
            records, stub = generator._stub_for_algorithm("ManyOptionalOutputs", 1)

        self.assertEqual(records, [])
        self.assertTrue(stub.endswith("-> Any: ..."))

    def test_check_detects_stale_stub(self):
        with tempfile.TemporaryDirectory() as temporary_directory, patch.dict(sys.modules, _mantid_modules()):
            output = Path(temporary_directory) / "simpleapi.pyi"
            argv = ["generate_simpleapi_stubs.py", "ConvertUnits", "--output", str(output)]
            with patch.object(generator, "_format_with_ruff", side_effect=lambda contents: contents), patch.object(sys, "argv", argv):
                self.assertEqual(generator.main(), 0)

            with (
                patch.object(generator, "_format_with_ruff", side_effect=lambda contents: contents),
                patch.object(sys, "argv", [*argv, "--check"]),
            ):
                self.assertEqual(generator.main(), 0)

            output.write_text("stale stub\n")
            error_output = io.StringIO()
            with (
                patch.object(generator, "_format_with_ruff", side_effect=lambda contents: contents),
                patch.object(sys, "argv", [*argv, "--check"]),
                contextlib.redirect_stderr(error_output),
            ):
                self.assertEqual(generator.main(), 1)
            self.assertIn("Stubs are stale", error_output.getvalue())

    def test_fit_signature_and_return_cover_inout_properties(self):
        fit = _FittingAlgorithm(
            {
                "Function": _property("Function", "Function", valid="Required", class_name="FunctionProperty"),
                "InputWorkspace": _property("InputWorkspace", "Workspace", valid="Required", class_name="WorkspaceProperty"),
                "CostFunction": _property("CostFunction", "string"),
                "OutputStatus": _property("OutputStatus", "string"),
                "OutputChi2overDoF": _property("OutputChi2overDoF", "number", class_name="FloatProperty"),
            },
            outputs=["OutputStatus", "OutputChi2overDoF"],
            inouts=["Function", "CostFunction"],
            inputs=["CostFunction"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: fit)
        generator._workspace_imports.update({"Workspace": "mantid.api"})
        generator._workspace_import_names.update({"Workspace": "Workspace"})

        with patch.dict(sys.modules, modules):
            records, stub = generator._stub_for_fitting_algorithm("Fit", 1)

        self.assertIn("Function: str | FunctionWrapper | IFunction", stub)
        self.assertIn("InputWorkspace: Workspace | str | None", stub)
        self.assertIn("CostFunction: str | None = ...", stub)
        self.assertIn("**kwargs: Any", stub)
        self.assertIn("-> _FitReturns", stub)
        self.assertIn("Function: FunctionWrapper", "\n".join(records))
        self.assertIn("OutputChi2overDoF: float", "\n".join(records))

    def test_simultaneous_fit_returns_optional_output_variants(self):
        algorithm = _FittingAlgorithm(
            {
                "Function": _property("Function", "Function", valid="Required", class_name="FunctionProperty"),
                "InputWorkspace": _property("InputWorkspace", "Workspace", valid="Required", class_name="WorkspaceProperty"),
                "OutputWorkspace": _property("OutputWorkspace", "WorkspaceGroup", valid="Required", class_name="WorkspaceGroupProperty"),
                "OutputParameterWorkspace": _property(
                    "OutputParameterWorkspace", "ITableWorkspace", class_name="ITableWorkspaceProperty", optional=True
                ),
                "OutputWorkspaceGroup": _property(
                    "OutputWorkspaceGroup", "WorkspaceGroup", class_name="WorkspaceGroupProperty", optional=True
                ),
            },
            outputs=["OutputWorkspace", "OutputParameterWorkspace", "OutputWorkspaceGroup"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)
        generator._workspace_imports.update({"Workspace": "mantid.api", "WorkspaceGroup": "mantid.api", "ITableWorkspace": "mantid.api"})
        generator._workspace_import_names.update(
            {"Workspace": "Workspace", "WorkspaceGroup": "WorkspaceGroup", "ITableWorkspace": "ITableWorkspace"}
        )

        with patch.dict(sys.modules, modules):
            records, stub = generator._stub_for_fitting_algorithm("QENSFitSimultaneous", 1)

        self.assertEqual(len(records), 4)
        self.assertIn("_QENSFitSimultaneousReturns1 | _QENSFitSimultaneousReturns2", stub)
        self.assertIn("OutputParameterWorkspace: ITableWorkspace", "\n".join(records))
        self.assertIn("OutputWorkspaceGroup: WorkspaceGroup", "\n".join(records))

    def test_load_signature_keeps_filename_and_output_positional(self):
        algorithm = _PropertyAlgorithm(
            {
                "Filename": _property("Filename", "list of str lists", valid="Required", class_name="MultipleFileProperty"),
                "OutputWorkspace": _property("OutputWorkspace", "Workspace", valid="Required", class_name="WorkspaceProperty"),
                "LoaderName": _property("LoaderName", "string"),
                "LoaderVersion": _property("LoaderVersion", "number", class_name="IntProperty"),
            },
            inputs=["Filename"],
            outputs=["OutputWorkspace", "LoaderName", "LoaderVersion"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)
        generator._workspace_imports["Workspace"] = "mantid.api"
        generator._workspace_import_names["Workspace"] = "Workspace"

        with patch.dict(sys.modules, modules):
            decorators, stubs = generator._stub_for_custom_algorithm("Load", 1)

        self.assertEqual(decorators, [])
        self.assertIn("Filename: str | list[str], OutputWorkspace: Workspace | str | None = ...", stubs[0])
        self.assertIn("**kwargs: Any) -> Workspace", stubs[0])

    def test_start_live_data_requires_instrument_and_types_static_options(self):
        algorithm = _PropertyAlgorithm(
            {
                "Instrument": _property("Instrument", "string"),
                "Facility": _property("Facility", "string"),
                "Listener": _property("Listener", "string", allowed=("FakeEventDataListener", "FileEventDataListener")),
                "AccumulationWorkspace": _property("AccumulationWorkspace", "Workspace", class_name="WorkspaceProperty"),
            },
            inputs=["FromNow", "Instrument", "Facility", "Listener"],
            outputs=["AccumulationWorkspace"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)
        generator._workspace_imports["Workspace"] = "mantid.api"
        generator._workspace_import_names["Workspace"] = "Workspace"
        algorithm.properties["FromNow"] = _property("FromNow", "boolean")

        with patch.dict(sys.modules, modules):
            _, stubs = generator._stub_for_custom_algorithm("StartLiveData", 1)

        self.assertIn("Instrument: str, *", stubs[0])
        self.assertIn("FromNow: bool | str | None = ...", stubs[0])
        self.assertIn("Listener: Literal['FakeEventDataListener', 'FileEventDataListener'] | None = ...", stubs[0])
        self.assertIn("**kwargs: Any) -> Any", stubs[0])

    def test_cutmd_overloads_single_and_multiple_workspace_inputs(self):
        algorithm = _PropertyAlgorithm(
            {
                "InputWorkspace": _property("InputWorkspace", "IMDWorkspace", valid="Required", class_name="IMDWorkspaceProperty"),
                "Projection": _property("Projection", "TableWorkspace", class_name="ITableWorkspaceProperty"),
                "P1Bin": _property("P1Bin", "dbl list", class_name="FloatArrayProperty"),
                "OutputWorkspace": _property("OutputWorkspace", "IMDWorkspace", class_name="IMDWorkspaceProperty"),
            },
            inputs=["InputWorkspace", "Projection", "P1Bin"],
            outputs=["OutputWorkspace"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)
        generator._workspace_imports.update({"IMDWorkspace": "mantid.api", "TableWorkspace": "mantid.api"})
        generator._workspace_import_names.update({"IMDWorkspace": "IMDWorkspace", "TableWorkspace": "TableWorkspace"})

        with patch.dict(sys.modules, modules):
            decorators, stubs = generator._stub_for_custom_algorithm("CutMD", 1)

        self.assertEqual(decorators, ["@overload", "@overload"])
        self.assertIn("InputWorkspace: IMDWorkspace | str", stubs[0])
        self.assertIn("InputWorkspace: list[IMDWorkspace | str]", stubs[1])
        self.assertIn("PBins: list[list[float]] | tuple[list[float], ...] | None = ...", stubs[0])
        self.assertIn("OutputWorkspace: str | None = ...", stubs[0])
        self.assertIn("-> list[IMDWorkspace]", stubs[1])

    def test_rename_workspace_overloads_output_assignment_convention(self):
        algorithm = _PropertyAlgorithm(
            {
                "InputWorkspace": _property("InputWorkspace", "Workspace", valid="Required", class_name="WorkspaceProperty"),
                "RenameMonitors": _property("RenameMonitors", "boolean", class_name="BoolPropertyWithValue"),
                "OverwriteExisting": _property("OverwriteExisting", "boolean", class_name="BoolPropertyWithValue"),
                "OutputWorkspace": _property("OutputWorkspace", "Workspace", class_name="WorkspaceProperty"),
            },
            inputs=["InputWorkspace", "RenameMonitors", "OverwriteExisting"],
            outputs=["OutputWorkspace"],
        )
        modules = _mantid_modules()
        modules["mantid.api"].AlgorithmManager = types.SimpleNamespace(createUnmanaged=lambda _name, _version: algorithm)
        generator._workspace_imports["Workspace"] = "mantid.api"
        generator._workspace_import_names["Workspace"] = "Workspace"

        with patch.dict(sys.modules, modules):
            decorators, stubs = generator._stub_for_custom_algorithm("RenameWorkspace", 1)

        self.assertEqual(decorators, ["@overload", "@overload"])
        self.assertIn("OutputWorkspace: Workspace | str | None = ...", stubs[0])
        self.assertIn("RenameMonitors: bool | str | None = ...", stubs[0])
        self.assertIn("*, OutputWorkspace: Workspace | str | None = ...", stubs[1])
        self.assertTrue(all(stub.endswith("-> Workspace: ...") for stub in stubs))


if __name__ == "__main__":
    unittest.main()
