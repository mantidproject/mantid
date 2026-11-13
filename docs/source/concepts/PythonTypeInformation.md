(PythonTypeInformation)=

# Python editor type information

Mantid includes Python type information files to help compatible editors
understand the Mantid API. These files do not change how Mantid runs Python
code.

## What is an LSP?

The Language Server Protocol (LSP) is a standard way for an editor to ask a
language server to analyze code. A Python language server can use type
information to provide completions, show parameter and return types, and report
some mistakes while you edit.

## Mantid's type files

Mantid ships a `.pyi` type file alongside the `mantid.simpleapi` module. It
describes algorithm parameters and outputs. For example, an LSP can use the
declaration for `ConvertUnits` to suggest valid `Target` values and show its
required parameters. Type files for compiled Mantid objects are a separate
workstream and will extend this coverage.

The file is located at `mantid/simpleapi.pyi` in the installed Python package
and is included in Mantid's conda package. When using a developer build, point
your editor at the Python environment configured for that build so it can find
both Mantid's Python sources and compiled modules.

To use the declarations, configure Pyright in an editor that supports Python
language servers and select the same Python environment that you use to run
Mantid. For a developer build, configure Pyright's import paths to include
`Framework/PythonInterface` and the build's `bin` directory so it can find the
Python package and compiled extension modules. Pyright is also used to check
the declarations in Mantid's development tests.

Coverage is being expanded. Type information for objects from compiled
`mantid.kernel`, `mantid.api`, and `mantid.dataobjects` bindings is a separate
workstream, so some algorithm input and return types may not yet show full
object details.

## Workbench editor and console

The Workbench script editor uses QScintilla, which does not read `.pyi` files
directly. Its current completion and call-tip support is separate from LSP
support, so installing Mantid's type files does not by itself change the script
editor.

The IPython console uses a different completion path. IPython can use Jedi for
static completion, and Jedi understands stub files. Mantid Workbench's console
behavior with Mantid's type files has not yet been verified.
