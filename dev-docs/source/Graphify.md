(Graphify)=

# Graphify: code knowledge graph

[Graphify](https://github.com/Graphify-Labs/graphify) builds a queryable graph of source code.
Mantid's configuration limits the graph to the framework code only:
the source that ends up in the `mantid` conda package, which is `Framework/` and `scripts/`.
The GUI code (`qt/`), documentation and tests are not included.

The graph holds classes, functions, includes and imports, calls and references,
grouped into communities of closely connected code.
It is useful for questions such as "what is connected to `MatrixWorkspace`?"
or "how does `scripts/` use the Python API in `Framework/`?".

Mantid configures graphify for a **code-only** run.
Code is parsed locally with tree-sitter,
so building the graph makes no LLM calls and needs no API key.
The one exception is naming communities, described under [Usage](#usage).

## Installing

`graphifyy` (note the double `y`) is part of the pixi developer environment,
so `graphify` is available inside `pixi shell` or through `pixi run graphify`.
See the [installation guide](https://docs.graphify.com/open-source/installation) for other ways to install it.

## Configuration

Two files in the root of the repository control what goes into the graph.

`.graphifyignore`
: Uses [gitignore-style patterns](https://docs.graphify.com/open-source/configuration/ignoring-files).
It limits the graph to the framework code (`Framework/` and `scripts/`).
Within those folders it also excludes tests, documentation, images,
and generated or vendored code (gSOAP for ICat, FlatBuffers for LiveData).

`.graphifyrc`
: Holds [project settings](https://docs.graphify.com/open-source/configuration/graphifyrc).
It tells graphify to parse `.h` and `.hxx` files as C++.

## Usage

Run these from the root of the source tree:

```sh
graphify extract . --code-only      # build graphify-out/graph.json
graphify cluster-only . --no-label  # write GRAPH_REPORT.md
graphify update .                   # re-extract only the files that changed
graphify query "how does DownloadInstrument work"
graphify path "DownloadInstrument" "InternetHelper"
graphify explain "MatrixWorkspace"
graphify export html                # interactive graphify-out/graph.html
graphify hook install               # rebuild the graph automatically after commits
```

The output goes into `graphify-out/`, which is ignored by git.
`GRAPH_REPORT.md` summarises the most connected nodes, the communities and suggested questions.

Without `--no-label`, `graphify cluster-only` names the communities with an LLM.
It uses an API key if one is set, and otherwise falls back to the `claude` command-line tool if it is installed.
For about 2,500 communities this takes several minutes and uses LLM tokens.
Pass `--no-label`, or set `GRAPHIFY_BACKEND=none`, to keep the whole run local.
For graphs over 5,000 nodes, which includes Mantid's, `graph.html` shows one node per community, not one per symbol.

`graphify hook install` adds git hooks (post-commit, post-checkout, post-merge and post-rewrite)
that re-extract the changed code files and rebuild the graph.
If a hook of the same name already exists, graphify adds to it and does not replace it.
Remove the hooks with `graphify hook uninstall`, and check them with `graphify hook status`.

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
