#!/bin/bash -ex

CONDA_PATH="$(which -a conda | tail -n 1)" # gets all conda executables on system, turns the last (not this wrapper script).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="${SCRIPT_DIR}/${CONDA_SUBDIR}"

# Extract env type and package name from command args (if present)
if [[ "$*" == *"create"* ]]; then
  case "$*" in
    *"_build_env"*)
      ENV_TYPE="build"
      ;;
    *"_h_env"*)
      ENV_TYPE="host"
      ;;
    *)
      ENV_TYPE=""
      ;;
  esac

  # Extract the package name from the prefix
  PACKAGE_NAME=""
  if [[ "$*" =~ .*/(mantid[^/]*)/.*_(build|h)_env.* ]]; then
    PACKAGE_NAME="${BASH_REMATCH[1]}"
  fi

  # Set CONDARC if we have both a relevant env type, and package name
  if [[ -n "$ENV_TYPE" && -n "$PACKAGE_NAME" ]]; then
    CONDARC_PATH="${OUTPUT_DIR}/${PACKAGE_NAME}-${ENV_TYPE}.yaml"
    if [[ -f "$CONDARC_PATH" ]]; then
      export CONDARC="$CONDARC_PATH"
    else
      echo "No condarc found at: $CONDARC_PATH" >&2
    fi
  fi
fi

exec "${CONDA_PATH}" "$@"
