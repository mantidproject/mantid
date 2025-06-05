#!/bin/bash -ex

CONDA_LOCK_DIR=$1
PLATFORM=$2
ENABLE_BUILD_MANTID=$3
ENABLE_BUILD_QT=$4
ENABLE_BUILD_WORKBENCH=$5

LOCKFILE_DIR="${CONDA_LOCK_DIR}/${PLATFORM}"
OUTPUT_DIR="${LOCKFILE_DIR}"

generate_condarc() {
    local package=$1
    local env_type=$2
    local lockfile="${LOCKFILE_DIR}/${package}-${env_type}-lockfile.yml"
    local output="${OUTPUT_DIR}/${package}-${env_type}.condarc.yaml"

    if [[ ! -f "$lockfile" ]]; then
        echo "Lockfile not found: $lockfile" >&2
        return
    fi

    echo "pinned_packages:" > "$output"
    conda-lock list --lockfile "$lockfile" | while read -r line; do
        pin=$(echo "$line" | cut -d= -f1-2)  # remove build string from line, not used by condarc
        echo "  - $pin" >> "$output"
    done
}

source "$(conda info --base)/etc/profile.d/conda.sh"
# activate or create conda-lock env
conda activate conda-lock 2>/dev/null || {
    conda create -y -n conda-lock conda-lock
    conda activate conda-lock
}

if [[ "$ENABLE_BUILD_MANTID" == true ]]; then
    generate_condarc mantid build
    generate_condarc mantid host
fi

if [[ "$ENABLE_BUILD_QT" == true ]]; then
    generate_condarc mantidqt build
    generate_condarc mantidqt host
fi

if [[ "$ENABLE_BUILD_WORKBENCH" == true ]]; then
    generate_condarc mantidworkbench build
    generate_condarc mantidworkbench host
fi

conda deactivate
