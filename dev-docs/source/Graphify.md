(Graphify)=

# Graphify: code knowledge graph

[Graphify](https://github.com/Graphify-Labs/graphify) builds a queryable graph of source code.
Mantid's configuration limits the graph to the framework and GUI code:
`Framework/` and `scripts/`, which make up the `mantid` conda package,
and `qt/`, which makes up the `mantidqt` and `mantidworkbench` packages.
Documentation and tests are not included.

The graph holds classes, functions, includes and imports, calls and references,
grouped into communities of closely connected code.
It is useful for questions such as "what is connected to `MatrixWorkspace`?"
or "which parts of the GUI in `qt/` use the Python API in `Framework/`?".

Mantid configures graphify for a **code-only** run.
Code is parsed locally with tree-sitter,
so building the graph makes no LLM calls and needs no API key.
The one exception is naming communities, described under [Usage](#usage).

## Installing

The name of the package is `graphifyy` (note the double `y`) and is not part of the developer environment.
It can be installed `pixi add --global graphifyy` then it can be used inside `pixi shell` or through `pixi run graphify`.
See the [installation guide](https://docs.graphify.com/open-source/installation) for other ways to install it.

## Configuration

Two files in the root of the repository control what goes into the graph.

`.graphifyignore`
: Uses [gitignore-style patterns](https://docs.graphify.com/open-source/configuration/ignoring-files).
It limits the graph to `Framework/`, `scripts/` and `qt/`.
Within those folders it also excludes tests, documentation, images,
and generated or vendored code
(gSOAP for ICat, FlatBuffers for LiveData, the Qt Solutions property browser and generated Qt resources).

`.graphifyrc`
: Holds [project settings](https://docs.graphify.com/open-source/configuration/graphifyrc).
It tells graphify to parse `.h` and `.hxx` files as C++.

## Usage

See the [CLI quickstart](https://docs.graphify.com/open-source/cli-quickstart)
and the [CLI reference](https://docs.graphify.com/open-source/configuration/cli) for all commands and options.

## AI coding assistants

`graphify install` adds a `/graphify` skill to coding assistants such as Claude Code,
which can then answer questions from the graph.
See [using graphify with coding assistants](https://docs.graphify.com/open-source/vibe-coding).
Contributions made with an AI assistant must follow the
[AI policy](https://github.com/mantidproject/mantid/blob/main/AI_POLICY.md).

## Limitations

- Algorithms are created by name (`AlgorithmManager::Instance().create("Name")`),
  so those calls do not appear as edges in the graph.
  Use `grep` to find them.
- Names are matched without their namespace.
  As a result, `std::string` references are attached to an unrelated `string` enum value,
  which then appears as the most connected node.
- Graphify does not run the C++ preprocessor.
  Mantid's macros (`DECLARE_ALGORITHM`, `PARALLEL_FOR_IF` and the export macros)
  therefore cause parse errors in many files,
  but very few symbols are lost.
- The graph is a snapshot of the code when it was built.
  Run `graphify update .` after changing code, or install the git hooks.
