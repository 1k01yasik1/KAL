#!/usr/bin/env python3
"""Generate sparse Graphviz graphs for benchmarking the TSP solver.

The produced graphs are undirected (symmetric weights) by default, so they are
compatible with the travelling salesman solvers shipped with this project.  A
Hamiltonian cycle is embedded explicitly, which guarantees that at least one
tour exists even though the graph remains far from complete (every vertex keeps
its degree below the requested maximum).
"""

from __future__ import annotations

import argparse
import random
from pathlib import Path
from typing import Dict, Iterable, List, Set, Tuple


def _validate_args(args: argparse.Namespace) -> None:
    if args.max_degree < 2:
        raise SystemExit("--max-degree must be at least 2 to keep the base cycle")
    if args.min_degree < 0:
        raise SystemExit("--min-degree cannot be negative")
    if args.min_degree > args.max_degree:
        raise SystemExit("--min-degree cannot exceed --max-degree")
    if args.weight_min <= 0 or args.weight_max <= 0:
        raise SystemExit("weights must be positive")
    if args.weight_min > args.weight_max:
        raise SystemExit("--weight-min cannot exceed --weight-max")
    if not args.sizes:
        raise SystemExit("at least one graph size is required")


def _parse_sizes(text: str) -> List[int]:
    sizes: List[int] = []
    for chunk in text.split(","):
        chunk = chunk.strip()
        if not chunk:
            continue
        try:
            value = int(chunk)
        except ValueError as exc:  # pragma: no cover - user error path
            raise argparse.ArgumentTypeError(f"invalid size value: {chunk!r}") from exc
        if value < 1:
            raise argparse.ArgumentTypeError("graph sizes must be positive")
        sizes.append(value)
    return sizes


def _default_sizes() -> List[int]:
    return [3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000]


def _format_weight(value: float) -> str:
    return f"{value:.6f}"


def _ensure_directory(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def _choose_target_degree(rng: random.Random, min_degree: int, max_degree: int, current: int) -> int:
    target = rng.randint(min_degree, max_degree)
    return max(target, current)


def _build_undirected_edges(
    n: int,
    max_degree: int,
    min_degree: int,
    rng: random.Random,
) -> List[Set[int]]:
    adjacency: List[Set[int]] = [set() for _ in range(n)]

    # Add a Hamiltonian cycle to guarantee at least one valid tour.
    for i in range(n):
        j = (i + 1) % n
        adjacency[i].add(j)
        adjacency[j].add(i)

    for vertex in range(n):
        target = _choose_target_degree(rng, min_degree, max_degree, len(adjacency[vertex]))
        attempts = 0
        limit = max(1000, 10 * n)  # Avoid infinite loops if the graph is saturated.
        while len(adjacency[vertex]) < target and attempts < limit:
            neighbour = rng.randrange(n)
            attempts += 1
            if neighbour == vertex:
                continue
            if len(adjacency[vertex]) >= max_degree or len(adjacency[neighbour]) >= max_degree:
                continue
            if neighbour in adjacency[vertex]:
                continue
            adjacency[vertex].add(neighbour)
            adjacency[neighbour].add(vertex)
        # No need to warn when the loop stops: saturation is expected near the limit.
    return adjacency


def _write_graphviz(
    path: Path,
    adjacency: List[Set[int]],
    rng: random.Random,
    weight_min: float,
    weight_max: float,
) -> None:
    weights: Dict[Tuple[int, int], float] = {}
    for u, neighbours in enumerate(adjacency):
        for v in neighbours:
            if u < v:
                if weight_min == weight_max:
                    weight = weight_min
                else:
                    weight = rng.uniform(weight_min, weight_max)
                weights[(u, v)] = weight

    with path.open("w", encoding="utf-8") as fh:
        fh.write("graph SparseTSP {\n")
        for vertex in range(len(adjacency)):
            fh.write(f"  \"{vertex}\";\n")
        for (u, v), weight in sorted(weights.items()):
            fh.write(f"  \"{u}\" -- \"{v}\" [weight={_format_weight(weight)}];\n")
        fh.write("}\n")


def generate_graph(
    size: int,
    output_dir: Path,
    rng: random.Random,
    max_degree: int,
    min_degree: int,
    weight_min: float,
    weight_max: float,
) -> Path:
    adjacency = _build_undirected_edges(size, max_degree, min_degree, rng)
    filename = output_dir / f"graph_{size}.gv"
    _write_graphviz(filename, adjacency, rng, weight_min, weight_max)
    return filename


def main(argv: Iterable[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="Generate sparse Graphviz graphs for benchmarking")
    parser.add_argument(
        "--sizes",
        type=_parse_sizes,
        default=_default_sizes(),
        help="Comma-separated list of graph sizes to generate (default: 3000-7000 step 500)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("lr4/data/generated_graphs"),
        help="Directory where Graphviz files will be written",
    )
    parser.add_argument(
        "--max-degree",
        type=int,
        default=15,
        help="Maximum degree per vertex (inclusive)",
    )
    parser.add_argument(
        "--min-degree",
        type=int,
        default=2,
        help="Minimum degree per vertex before random augmentation",
    )
    parser.add_argument(
        "--weight-min",
        type=float,
        default=1.0,
        help="Lower bound for edge weights",
    )
    parser.add_argument(
        "--weight-max",
        type=float,
        default=100.0,
        help="Upper bound for edge weights",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=12345,
        help="Random seed to make the generated graphs deterministic",
    )

    args = parser.parse_args(list(argv) if argv is not None else None)
    _validate_args(args)

    rng = random.Random(args.seed)
    _ensure_directory(args.output_dir)

    for size in args.sizes:
        graph_path = generate_graph(
            size=size,
            output_dir=args.output_dir,
            rng=rng,
            max_degree=args.max_degree,
            min_degree=args.min_degree,
            weight_min=args.weight_min,
            weight_max=args.weight_max,
        )
        print(f"Generated {size} vertices -> {graph_path}")


if __name__ == "__main__":
    main()
