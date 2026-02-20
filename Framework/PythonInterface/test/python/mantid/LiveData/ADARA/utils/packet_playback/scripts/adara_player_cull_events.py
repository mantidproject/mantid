#!/usr/bin/env python3
# ruff: noqa: E402  # Module level import not at top of file
"""
Shrink ADARA packet files by randomly down-sampling event packets.

- Uses UnixGlob.parse / multi_glob to resolve an input glob, consistent with adara_player.
- Uses Player._summarize to decide which files are metadata-only vs event-bearing.
- Copies metadata-only files unchanged to the output directory.
- For event-bearing files, rewrites them in the output directory with only a
  random fraction of event packets kept; all non-event packets are preserved.
"""

import argparse
import logging
import random
import shutil
import sys
from pathlib import Path

# Make ADARA-packet player utilities importable
SCRIPT_DIR = Path(__file__).resolve().parent
ADARA_UTILS_DIR = (
    SCRIPT_DIR.parents[2]  # ${REPO_BASE}/Framework/LiveData
    / "src"
    / "ADARA"
    / "utils"
    / "packet_playback"
)
sys.path.insert(0, str(ADARA_UTILS_DIR))

from packet_player import Player, Packet, UnixGlob  # module name from file: packet_player.py

logger = logging.getLogger("adara_player_cull_events")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="adara_player_cull_events",
        usage=(
            "adara_player_cull_events [options] OUTPUT_DIR INPUT_GLOB\n\n"
            "Examples:\n"
            "  adara_player_cull_events out 'runs/*.adara'\n"
            "  adara_player_cull_events out '/data/run{0012,0013}/*.adara'\n"
        ),
        description=("Reduce size of ADARA packet files by randomly culling event packets while preserving metadata packets."),
    )
    parser.add_argument(
        "output_dir",
        type=Path,
        help="Directory where reduced files will be written.",
    )
    parser.add_argument(
        "input_glob",
        type=str,
        help=("Input-files glob (same syntax as adara_player), e.g. '/data/run0012/*.adara' or '/data/run{0012,0013}/*.adara'."),
    )
    parser.add_argument(
        "-k",
        "--keep-fraction",
        type=float,
        default=0.1,
        help=("Fraction of event packets to keep (0.0 - 1.0). For example, 0.1 keeps ~10%% of events. Default: 0.1."),
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=None,
        help="Random seed for reproducible down-sampling (optional).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be done, but do not write any output files.",
    )
    return parser.parse_args()


def resolve_input_files(glob_pattern: str) -> list[Path]:
    """
    Use UnixGlob.parse and UnixGlob.multi_glob to resolve the input glob
    in the same way adara_player does.
    """
    base_dir, patterns = UnixGlob.parse(glob_pattern)  #
    files_iter = UnixGlob.multi_glob(base_dir, patterns)  #
    files = sorted(p for p in files_iter if p.is_file())
    return files


def classify_files(player: Player, files: list[Path]):
    """
    Run Player._summarize to obtain per-file summaries and classify
    them into metadata-only vs event-bearing sets.
    """
    summaries = player._summarize(files)  # internal helper, returns _PacketFileSummary list

    metadata_files: list[Path] = []
    event_files: list[Path] = []

    for s in summaries:
        if s.is_events_file:  # True if this file contains any event-bearing packet types.
            event_files.append(s.file_path)
        else:
            metadata_files.append(s.file_path)

    return metadata_files, event_files


def copy_metadata_files(metadata_files: list[Path], output_dir: Path, dry_run: bool) -> None:
    for src in metadata_files:
        dst = output_dir / src.name
        logger.info("Metadata-only file:\n  %s\n    -> %s", src, dst)
        if dry_run:
            continue
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)


def downsample_event_file(
    src: Path,
    dst: Path,
    keep_fraction: float,
    dry_run: bool,
) -> tuple[int, int]:
    """
    Read packets from src and write to dst, randomly keeping only a fraction
    of event packets (as defined by Packet.Type.EVENT_TYPES), and writing
    all non-event packets unchanged.
    Returns (total_event_packets, kept_event_packets).

    In dry-run mode, performs the same random selection logic but does not
    write any output file; it still reports both totals.
    """
    total_events = 0
    kept_events = 0

    logger.info(
        "Event file:\n  %s\n    -> %s (keep_fraction=%.3f)",
        src,
        dst,
        keep_fraction,
    )

    if dry_run:
        # Simulate random culling without writing output.
        with open(src, "rb", buffering=0) as f:
            for pkt in Player.iter_file(f, header_only=False, source=str(src)):
                if Packet.Type.is_event_type(pkt.packet_type):
                    total_events += 1
                    if random.random() <= keep_fraction:
                        kept_events += 1

        frac = (kept_events / total_events) if total_events else 0.0
        logger.info(
            "  (dry-run) events: %d total, %d would be kept (%.3f fraction)",
            total_events,
            kept_events,
            frac,
        )
        return total_events, kept_events

    # Real rewrite mode: actually write the selected packets.
    dst.parent.mkdir(parents=True, exist_ok=True)

    with open(src, "rb", buffering=0) as fin, open(dst, "wb") as fout:
        for pkt in Player.iter_file(fin, header_only=False, source=str(src)):
            is_event = Packet.Type.is_event_type(pkt.packet_type)
            if is_event:
                total_events += 1
                if random.random() <= keep_fraction:
                    kept_events += 1
                    Packet.to_file(fout, pkt)  # write header+payload unchanged
            else:
                # Non-event packets are always kept.
                Packet.to_file(fout, pkt)

    logger.info(
        "  events: %d total, %d kept (%.3f fraction)",
        total_events,
        kept_events,
        (kept_events / total_events) if total_events else 0.0,
    )
    return total_events, kept_events


def main():
    args = parse_args()

    # Basic sanity checks
    if not (0.0 <= args.keep_fraction <= 1.0):
        raise SystemExit("keep-fraction must be between 0.0 and 1.0")

    if args.seed is not None:
        random.seed(args.seed)

    # Initialize logging similarly to packet_player (console-level only here).
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )

    player = Player()  # uses Config and initializes internal filters etc.

    input_files = resolve_input_files(args.input_glob)
    if not input_files:
        logger.warning("No input files matched glob: %s", args.input_glob)
        return

    output_dir = args.output_dir.resolve()
    for src in input_files:
        if (output_dir / src.name).resolve() == src.resolve():
            raise SystemExit(f"output_dir '{output_dir}' would overwrite input file '{src}'. Choose a different directory.")

    logger.info("Found %d input files.", len(input_files))

    metadata_files, event_files = classify_files(player, input_files)
    logger.info(
        "Classified files: %d metadata-only, %d event-bearing.",
        len(metadata_files),
        len(event_files),
    )

    # Ensure output directory exists (or simulate in dry-run).
    if not args.dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    # 1) Copy metadata-only files unchanged.
    copy_metadata_files(metadata_files, output_dir, args.dry_run)

    # 2) Down-sample event-bearing files.
    for src in event_files:
        dst = output_dir / src.name
        downsample_event_file(
            src=src,
            dst=dst,
            keep_fraction=args.keep_fraction,
            dry_run=args.dry_run,
        )

    logger.info("Done.")


if __name__ == "__main__":
    main()
