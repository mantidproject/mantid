#!/usr/bin/env python3
"""Generate simpleapi stubs from a configured Mantid build."""

from __future__ import annotations

import argparse
import difflib
import importlib
import itertools
import keyword
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

_workspace_imports: dict[str, str] = {}
_workspace_import_names: dict[str, str] = {}
_used_workspace_types: set[str] = set()
_unsupported_property_types: set[str] = set()
_used_function_types = False
_used_array_aliases: set[str] = set()
# Mantid array property type -> (stub alias, scalar element type accepted as a length-one array)
_ARRAY_ALIASES = {
    "dbl list": ("_Float1DArray", "float"),
    "int list": ("_Int1DArray", "int"),
    "long list": ("_Int1DArray", "int"),
    "unsigned int list": ("_Int1DArray", "int"),
    "str list": ("_Str1DArray", None),
}
_ARRAY_ALIAS_DEFINITIONS = {
    "_Float1DArray": "Sequence[float] | numpy.ndarray",
    "_Int1DArray": "Sequence[int] | numpy.ndarray",
    "_Str1DArray": "Sequence[str]",
}
# Property::type() falls back to the compiler-specific typeid name for unsigned
# types, so identify those by the Python class the bindings export them as
# (mantid/kernel/src/Exports/PropertyWithValue.cpp and ArrayProperty.cpp),
# which is the same everywhere.
_UNNAMED_PROPERTY_CLASS_TYPES = {
    "UIntPropertyWithValue": "number",
    "ULongPropertyWithValue": "number",
    "ULongLongPropertyWithValue": "number",
    "UInt32ArrayProperty": "unsigned int list",
    "VectorUIntPropertyWithValue": "unsigned int list",
    "VectorULongPropertyWithValue": "unsigned int list",
    "VectorULongLongPropertyWithValue": "unsigned int list",
}
_MAX_OPTIONAL_OUTPUT_VARIANTS = 3
_FITTING_ALGORITHMS = {
    "Fit",
    "CalculateChiSquared",
    "EvaluateFunction",
    "QENSFitSimultaneous",
    "ConvolutionFitSimultaneous",
    "IqtFitSimultaneous",
}
_CUSTOM_ALGORITHMS = {"Load", "StartLiveData", "CutMD", "RenameWorkspace"}


def _property_type(prop) -> str:
    """Return a compiler-independent name for the property's value type."""
    return _UNNAMED_PROPERTY_CLASS_TYPES.get(type(prop).__name__, prop.type)


def _type_for(prop) -> str:
    """Map Mantid property metadata to a conservative Python type."""
    global _used_function_types
    prop_type = _property_type(prop)
    prop_class = type(prop).__name__

    if "Workspace" in prop_type or prop_class.endswith("WorkspaceProperty"):
        if prop_type in _workspace_imports:
            _used_workspace_types.add(prop_type)
            return f"{_workspace_import_names[prop_type]} | str"
        _unsupported_property_types.add(prop_type)
        return "Any"
    if prop_type == "IAlgorithm":
        _used_workspace_types.add(prop_type)
        return "IAlgorithm"
    if prop_type == "Function":
        _used_function_types = True
        return "str | IFunction"
    if prop_class == "MultipleFileProperty":
        return "str | list[str]"
    if prop_class == "FileProperty":
        # allowedValues() holds the accepted file extensions, not the accepted values.
        return "str"
    if prop_type == "string":
        allowed = list(prop.allowedValues)
        if allowed and len(allowed) <= 30:
            return f"Literal[{', '.join(repr(value) for value in allowed)}]"
        return "str"
    if prop_type == "number":
        if prop_class.startswith(("Int", "Long", "UInt", "ULong")):
            return "int"
        return "float"
    if prop_type == "boolean":
        return "bool"
    if prop_type in {"str list", "list of str lists"}:
        return "list[str]" if prop_type == "str list" else "list[list[str]]"
    if prop_type in {"dbl list", "int list", "long list", "unsigned int list"}:
        item_type = "float" if prop_type == "dbl list" else "int"
        return f"list[{item_type}]"
    if prop_type == "Dictionary":
        return "dict[str, Any]"
    if prop_type == "optional boolean":
        return "bool | None"
    _unsupported_property_types.add(prop_type)
    return "Any"


def _parameter_type_for(prop) -> str:
    """Widen a property type to everything simpleapi accepts as a call argument.

    Any ``str`` argument is passed to ``setPropertyValue`` and parsed into the
    property's type. Array properties accept any sequence or numpy array, and a
    bare scalar becomes a one-element array (see SequenceTypeHandler::set).
    """
    annotation = _type_for(prop)
    prop_type = _property_type(prop)
    if annotation.startswith("list[") and prop_type in _ARRAY_ALIASES:
        alias, scalar = _ARRAY_ALIASES[prop_type]
        _used_array_aliases.add(alias)
        return f"{alias} | {scalar} | str" if scalar else f"{alias} | str"
    if annotation == "list[list[str]]":
        _used_array_aliases.add("_Str1DArray")
        return "Sequence[_Str1DArray] | str"
    if annotation == "Any" or annotation.startswith("Literal["):
        return annotation
    members = annotation.split(" | ")
    if "str" in members:
        return annotation
    widened = [member for member in members if member != "None"]
    widened.append("str")
    if "None" in members:
        widened.append("None")
    return " | ".join(widened)


def _required(prop) -> bool:
    validity = prop.isValid
    if callable(validity):
        validity = validity()
    return bool(validity)


def _stub_for_algorithm(name: str, version: int) -> tuple[list[str], str]:
    from mantid.api import AlgorithmManager

    algorithm = AlgorithmManager.createUnmanaged(name, version)
    algorithm.initialize()

    properties = []
    seen = set()
    output_names = set(algorithm.outputProperties())
    for property_name in (*algorithm.mandatoryProperties(), *algorithm.outputProperties()):
        if property_name in seen:
            continue
        seen.add(property_name)
        prop = algorithm.getProperty(property_name)
        if not property_name.isidentifier() or keyword.iskeyword(property_name):
            raise ValueError(f"Cannot emit Python parameter for {name}.{property_name}")
        properties.append((prop, property_name in output_names))

    inputs = [(prop, is_output) for prop, is_output in properties if not is_output]
    outputs = [(prop, is_output) for prop, is_output in properties if is_output]
    required = [prop for prop, _ in inputs if _required(prop)]
    optional = [prop for prop, _ in inputs if not _required(prop)]

    parameters = [f"{prop.name}: {_parameter_type_for(prop)}" for prop in required]
    parameters.extend(f"{prop.name}: {_parameter_type_for(prop)} | None = ..." for prop in optional)
    parameters.append("*")
    parameters.extend(f"{prop.name}: {_parameter_type_for(prop)} | None = ..." for prop, _ in outputs)
    parameters.extend(
        [
            "Version: int = ...",
            "EnableLogging: bool = ...",
            "StoreInADS: bool = ...",
            "startProgress: float = ...",
            "endProgress: float = ...",
        ]
    )

    return_declarations, return_type = _return_annotation(name, [prop for prop, _ in outputs])

    return return_declarations, f"def {name}({', '.join(parameters)}) -> {return_type}: ..."


def _is_optional(prop) -> bool:
    optional = getattr(prop, "isOptional", False)
    if callable(optional):
        optional = optional()
    return bool(optional)


def _return_type_for(prop) -> str:
    if prop.type == "Function":
        return "FunctionWrapper"
    return _type_for(prop).removesuffix(" | str")


def _is_workspace_property(prop) -> bool:
    return "Workspace" in prop.type or type(prop).__name__.endswith("WorkspaceProperty")


def _return_annotation(name: str, outputs: list[object]) -> tuple[list[str], str]:
    """Describe simpleapi's scalar or namedtuple return for declared outputs."""
    if not outputs:
        return [], "None"

    optional_workspace_outputs = [prop for prop in outputs if _is_optional(prop) and _is_workspace_property(prop)]
    if len(optional_workspace_outputs) > _MAX_OPTIONAL_OUTPUT_VARIANTS:
        # Avoid exponential stub growth when a wrapper has many conditional outputs.
        return [], "Any"

    optional_names = {prop.name for prop in optional_workspace_outputs}
    optional_names_sorted = sorted(optional_names)
    field_variants = []
    for mask in range(1 << len(optional_names_sorted)):
        included_names = {property_name for index, property_name in enumerate(optional_names_sorted) if mask & (1 << index)}
        field_variants.append([prop for prop in outputs if prop.name not in optional_names or prop.name in included_names])

    namedtuple_variants = [fields for fields in field_variants if len(fields) > 1]
    declarations = []
    namedtuple_types = {}
    for index, fields in enumerate(namedtuple_variants, start=1):
        type_name = f"_{name}Returns" if len(namedtuple_variants) == 1 else f"_{name}Returns{index}"
        namedtuple_types[tuple(prop.name for prop in fields)] = type_name
        declarations.append(
            f"class {type_name}(NamedTuple):\n" + "\n".join(f"    {prop.name}: {_return_type_for(prop)}" for prop in fields) + "\n"
        )

    return_types = []
    for fields in field_variants:
        if not fields:
            type_name = "None"
        elif len(fields) == 1:
            type_name = _return_type_for(fields[0])
        else:
            type_name = namedtuple_types[tuple(prop.name for prop in fields)]
        if type_name not in return_types:
            return_types.append(type_name)
    return declarations, " | ".join(return_types)


def _fitting_return_type(prop) -> str:
    return _return_type_for(prop)


def _stub_for_fitting_algorithm(name: str, version: int) -> tuple[list[str], str]:
    """Emit a typed signature and return records for a simpleapi fitting wrapper."""
    from mantid.api import AlgorithmManager

    algorithm = AlgorithmManager.createUnmanaged(name, version)
    algorithm.initialize()

    output_names = list(algorithm.outputProperties())
    inout_names = list(algorithm.inoutProperties()) if name == "Fit" else []
    input_names = [property_name for property_name in algorithm.mandatoryProperties() if property_name not in output_names]
    if "Function" not in input_names:
        input_names.insert(0, "Function")
    if "InputWorkspace" not in input_names:
        input_names.insert(1, "InputWorkspace")

    properties = {property_name: algorithm.getProperty(property_name) for property_name in (*input_names, *output_names, *inout_names)}
    parameters = ["Function: str | FunctionWrapper | IFunction", "InputWorkspace: Workspace | str | None"]
    for property_name in input_names:
        if property_name in {"Function", "InputWorkspace"}:
            continue
        prop = properties[property_name]
        if not property_name.isidentifier() or keyword.iskeyword(property_name):
            raise ValueError(f"Cannot emit Python parameter for {name}.{property_name}")
        annotation = _parameter_type_for(prop)
        if not _required(prop):
            annotation += " | None"
            parameters.append(f"{property_name}: {annotation} = ...")
        else:
            parameters.append(f"{property_name}: {annotation}")

    parameters.append("*")
    for property_name in (*inout_names, *output_names):
        if property_name in input_names:
            continue
        prop = properties[property_name]
        if not property_name.isidentifier() or keyword.iskeyword(property_name):
            raise ValueError(f"Cannot emit Python parameter for {name}.{property_name}")
        annotation = _parameter_type_for(prop)
        if _is_optional(prop):
            annotation += " | None"
        parameters.append(f"{property_name}: {annotation} = ...")
    parameters.extend(["EnableLogging: bool = ...", "StoreInADS: bool = ...", "**kwargs: Any"])

    # Workspace output properties marked optional may be absent at runtime.
    # Emit a named-tuple variant for each possible combination so both
    # attribute access and positional unpacking retain accurate types.
    optional_output_names = {
        property_name
        for property_name in output_names
        if _is_optional(properties[property_name])
        and ("Workspace" in properties[property_name].type or type(properties[property_name]).__name__.endswith("WorkspaceProperty"))
    }
    sorted_optional_output_names = sorted(optional_output_names)
    variants: list[list[object]] = []
    for mask in range(1 << len(sorted_optional_output_names)):
        included_optional = {property_name for index, property_name in enumerate(sorted_optional_output_names) if mask & (1 << index)}
        fields = [
            properties[property_name]
            for property_name in output_names
            if property_name not in optional_output_names or property_name in included_optional
        ]
        fields.extend(properties[property_name] for property_name in inout_names)
        variants.append(fields)

    records: list[str] = []
    record_names = []
    for index, fields in enumerate(variants, start=1):
        record_name = f"_{name}Returns" if len(variants) == 1 else f"_{name}Returns{index}"
        record_names.append(record_name)
        records.append(
            f"class {record_name}(NamedTuple):\n" + "\n".join(f"    {prop.name}: {_fitting_return_type(prop)}" for prop in fields) + "\n"
        )

    return_type = record_names[0] if len(record_names) == 1 else " | ".join(record_names)
    return records, f"def {name}({', '.join(parameters)}) -> {return_type}: ..."


def _stub_for_custom_algorithm(name: str, version: int) -> tuple[list[str], list[str]]:
    """Emit signatures for simpleapi wrappers whose call behavior differs from algorithms."""
    from mantid.api import AlgorithmManager

    algorithm = AlgorithmManager.createUnmanaged(name, version)
    algorithm.initialize()
    properties = {property_name: algorithm.getProperty(property_name) for property_name in algorithm.mandatoryProperties()}
    properties.update({property_name: algorithm.getProperty(property_name) for property_name in algorithm.outputProperties()})

    controls = [
        "EnableLogging: bool = ...",
        "StoreInADS: bool = ...",
        "startProgress: float = ...",
        "endProgress: float = ...",
    ]

    if name == "Load":
        filename_type = _parameter_type_for(properties["Filename"])
        output_type = _parameter_type_for(properties["OutputWorkspace"])
        signature = (
            f"def Load(Filename: {filename_type}, OutputWorkspace: {output_type} | None = ..., *, "
            f"{', '.join(controls)}, **kwargs: Any) -> Workspace: ..."
        )
        return [], [signature]

    if name == "StartLiveData":
        parameters = ["Instrument: str", "*"]
        for property_name in algorithm.mandatoryProperties():
            if property_name == "Instrument":
                continue
            prop = properties[property_name]
            if not property_name.isidentifier() or keyword.iskeyword(property_name):
                raise ValueError(f"Cannot emit Python parameter for {name}.{property_name}")
            parameters.append(f"{property_name}: {_parameter_type_for(prop)} | None = ...")
        parameters.extend(controls)
        parameters.append("**kwargs: Any")
        return [], [f"def StartLiveData({', '.join(parameters)}) -> Any: ..."]

    if name == "CutMD":
        input_type = _parameter_type_for(properties["InputWorkspace"])
        output_type = _type_for(properties["OutputWorkspace"]).removesuffix(" | str")
        bin_parameters = []
        for property_name in algorithm.mandatoryProperties():
            if property_name == "InputWorkspace":
                continue
            prop = properties[property_name]
            if not property_name.isidentifier() or keyword.iskeyword(property_name):
                raise ValueError(f"Cannot emit Python parameter for {name}.{property_name}")
            bin_parameters.append(f"{property_name}: {_parameter_type_for(prop)} | None = ...")
        bin_parameters.append("PBins: list[list[float]] | tuple[list[float], ...] | None = ...")
        parameters = [f"InputWorkspace: {input_type}", "*", *bin_parameters]
        single_parameters = [*parameters, "OutputWorkspace: str | None = ...", *controls]
        multiple_parameters = [
            "InputWorkspace: list[IMDWorkspace | str]",
            "*",
            *bin_parameters,
            "OutputWorkspace: list[str] | None = ...",
            *controls,
        ]
        return ["@overload", "@overload"], [
            f"def CutMD({', '.join(single_parameters)}) -> {output_type}: ...",
            f"def CutMD({', '.join(multiple_parameters)}) -> list[{output_type}]: ...",
        ]

    if name == "RenameWorkspace":
        input_type = _parameter_type_for(properties["InputWorkspace"])
        output_type = _parameter_type_for(properties["OutputWorkspace"]) + " | None"
        rename_monitors = f"RenameMonitors: {_parameter_type_for(properties['RenameMonitors'])} | None = ..."
        overwrite_existing = f"OverwriteExisting: {_parameter_type_for(properties['OverwriteExisting'])} | None = ..."
        positional = [
            f"InputWorkspace: {input_type}",
            f"OutputWorkspace: {output_type} = ...",
            rename_monitors,
            "*",
            overwrite_existing,
        ]
        assigned = [
            f"InputWorkspace: {input_type}",
            rename_monitors,
            overwrite_existing,
            "*",
            f"OutputWorkspace: {output_type} = ...",
        ]
        return ["@overload", "@overload"], [
            f"def RenameWorkspace({', '.join([*positional, *controls])}) -> Workspace: ...",
            f"def RenameWorkspace({', '.join([*assigned, *controls])}) -> Workspace: ...",
        ]

    raise ValueError(f"No custom simpleapi stub implementation for {name}")


def _format_with_ruff(contents: str) -> str:
    """Format generated stub text using the repository's Ruff pre-commit hook."""
    repo_root = Path(__file__).parents[2]
    hook_runner = shutil.which("prek") or shutil.which("pre-commit")
    if hook_runner is None:
        raise RuntimeError("Stub generation requires prek or pre-commit to run the configured ruff-format hook")

    with tempfile.NamedTemporaryFile(
        mode="w", suffix=".pyi", prefix="_simpleapi_generated_", dir=repo_root / "tools/python_typing", delete=False
    ) as temporary:
        temporary.write(contents)
        temporary_path = Path(temporary.name)

    try:
        relative_path = temporary_path.relative_to(repo_root)
        result = subprocess.run(
            [hook_runner, "run", "ruff-format", "--files", str(relative_path)],
            cwd=repo_root,
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode:
            raise RuntimeError(f"Ruff formatting failed:\n{result.stdout}{result.stderr}")
        return temporary_path.read_text()
    finally:
        temporary_path.unlink(missing_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("algorithms", nargs="*", help="specific algorithm names to emit (default: ConvertUnits)")
    parser.add_argument("--all", action="store_true", help="emit all registered algorithms")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).parents[2] / "Framework/PythonInterface/mantid/simpleapi.pyi",
    )
    parser.add_argument("--check", action="store_true", help="fail if output differs instead of writing it")
    args = parser.parse_args()

    # Importing simpleapi loads Mantid's registered Python and C++ algorithms.
    import mantid.simpleapi
    from mantid.api import AlgorithmFactory

    for module_name in ("mantid.api", "mantid.dataobjects"):
        module = importlib.import_module(module_name)
        for property_type in (
            "Workspace",
            "MatrixWorkspace",
            "WorkspaceGroup",
            "MDEventWorkspace",
            "IMDEventWorkspace",
            "IMDHistoWorkspace",
            "IMDWorkspace",
            "EventWorkspace",
            "IEventWorkspace",
            "ITableWorkspace",
            "TableWorkspace",
            "IPeaksWorkspace",
            "PeaksWorkspace",
            "GroupingWorkspace",
            "OffsetsWorkspace",
            "MaskWorkspace",
            "Workspace2D",
        ):
            if hasattr(module, property_type):
                _workspace_imports[property_type] = module_name
                _workspace_import_names[property_type] = property_type

    # The algorithm property type names a concrete template, while Python exposes
    # its common interface as IMDEventWorkspace.
    if hasattr(importlib.import_module("mantid.api"), "IMDEventWorkspace"):
        _workspace_imports["MDEventWorkspace"] = "mantid.api"
        _workspace_import_names["MDEventWorkspace"] = "IMDEventWorkspace"
    if hasattr(importlib.import_module("mantid.api"), "IAlgorithm"):
        _workspace_imports["IAlgorithm"] = "mantid.api"
        _workspace_import_names["IAlgorithm"] = "IAlgorithm"

    registered = AlgorithmFactory.getRegisteredAlgorithms(True)
    if args.all and args.algorithms:
        parser.error("Use either --all or explicit algorithm names")
    if args.all:
        specialized = set(mantid.simpleapi.__SPECIALIZED_FUNCTIONS__)
        algorithm_names = sorted((set(registered) - specialized) | _FITTING_ALGORITHMS | _CUSTOM_ALGORITHMS)
    else:
        algorithm_names = sorted(set(args.algorithms or ["ConvertUnits"]))

    declarations = []
    return_declarations = []
    for name in algorithm_names:
        versions = registered.get(name)
        if not versions:
            parser.error(f"Algorithm is not registered: {name}")
        if name in _FITTING_ALGORITHMS:
            records, declaration = _stub_for_fitting_algorithm(name, max(versions))
            return_declarations.extend(records)
            declarations.append(declaration)
        elif name in _CUSTOM_ALGORITHMS:
            decorators, custom_declarations = _stub_for_custom_algorithm(name, max(versions))
            decorators.extend("" for _ in range(len(custom_declarations) - len(decorators)))
            declarations.extend(
                f"{decorator}\n{declaration}" if decorator else declaration
                for decorator, declaration in zip(decorators, custom_declarations)
            )
        else:
            records, declaration = _stub_for_algorithm(name, max(versions))
            return_declarations.extend(records)
            declarations.append(declaration)

    imports = sorted(
        {
            module_name: sorted(
                {_workspace_import_names[name] for name in _used_workspace_types if _workspace_imports[name] == module_name}
            )
            for module_name in {_workspace_imports[name] for name in _used_workspace_types}
        }.items()
    )
    # Flatten the deterministic module-to-symbol mapping into stub import lines.
    import_lines = [
        f"from {module_name} import (\n" + "\n".join(f"    {name}," for name in names) + "\n)" for module_name, names in imports
    ]
    if return_declarations or _used_function_types:
        import_lines.extend(["from mantid.api import IFunction", "from mantid.fitfunctions import FunctionWrapper"])
    if _unsupported_property_types:
        print(
            "Properties emitted as Any for unsupported Mantid types: " + ", ".join(sorted(_unsupported_property_types)),
            file=sys.stderr,
        )
    alias_imports = []
    if _used_array_aliases:
        alias_imports = ["from collections.abc import Sequence"]
        if any("numpy." in _ARRAY_ALIAS_DEFINITIONS[alias] for alias in _used_array_aliases):
            import_lines.insert(0, "import numpy")
    alias_declarations = [f"type {alias} = {_ARRAY_ALIAS_DEFINITIONS[alias]}" for alias in sorted(_used_array_aliases)]
    contents = "\n".join(
        [
            "# Generated by tools/python_typing/generate_simpleapi_stubs.py.",
            "# Run from a configured Mantid build environment; do not edit by hand.",
            *alias_imports,
            "from typing import Any, Literal, NamedTuple, overload",
            *import_lines,
            "",
            *alias_declarations,
            *return_declarations,
            *declarations,
            "def __getattr__(name: str) -> Any: ...",
            "",
        ]
    )
    contents = _format_with_ruff(contents)

    if args.check:
        current = args.output.read_text() if args.output.exists() else ""
        if current != contents:
            print(f"Stubs are stale: {args.output}", file=sys.stderr)
            diff = difflib.unified_diff(
                current.splitlines(keepends=True), contents.splitlines(keepends=True), "committed", "generated", n=1
            )
            sys.stderr.writelines(itertools.islice(diff, 200))
            return 1
        return 0

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(contents)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
