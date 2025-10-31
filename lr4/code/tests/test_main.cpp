#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

#include "../ant_colony_solver.h"
#include "../graph.h"

using lr4::AntColonyParameters;
using lr4::AntColonySolver;
using lr4::Graph;
using lr4::TourResult;

void TestGraphParsing() {
  std::istringstream input(R"(digraph G {
    "1" -> "2" [weight=3.5];
    "2" -> "1" [weight=3.5];
    2 -> 3 [label=7];
    3 -> 2 [label=7];
  })");
  Graph graph = Graph::FromGraphviz(input);
  assert(graph.VertexCount() == 3);
  assert(std::fabs(graph.Weight(0, 1) - 3.5) < 1e-9 || std::fabs(graph.Weight(1, 0) - 3.5) < 1e-9);
  auto canonical = graph.CanonicalizeTour({0, 1, 2, 0});
  assert(canonical.front() == canonical.back());
}

void TestSequentialSolver() {
  std::istringstream input(R"(digraph G {
    A -> B [weight=1];
    B -> A [weight=1];
    A -> C [weight=5];
    C -> A [weight=5];
    B -> C [weight=2];
    C -> B [weight=2];
  })");
  Graph graph = Graph::FromGraphviz(input);
  AntColonySolver solver(graph);
  AntColonyParameters params;
  params.ants = 30;
  params.iterations = 50;
  params.alpha = 1.0;
  params.beta = 5.0;
  params.evaporation = 0.3;
  params.q = 50.0;
  params.seed = 2024;
  TourResult result = solver.RunSequential(params);
  assert(std::isfinite(result.best_length));
  assert(!result.best_paths.empty());
}

void TestParallelSolverAgreement() {
  std::istringstream input(R"(digraph G {
    A -> B [weight=4];
    B -> A [weight=4];
    A -> C [weight=1];
    C -> A [weight=1];
    B -> C [weight=3];
    C -> B [weight=3];
  })");
  Graph graph = Graph::FromGraphviz(input);
  AntColonySolver solver(graph);
  AntColonyParameters params;
  params.ants = 40;
  params.iterations = 80;
  params.alpha = 1.2;
  params.beta = 5.0;
  params.evaporation = 0.2;
  params.q = 50.0;
  params.seed = 1337;
  TourResult seq = solver.RunSequential(params);
  TourResult par = solver.RunParallel(params, 4);
  assert(std::isfinite(seq.best_length));
  assert(std::isfinite(par.best_length));
  assert(!seq.best_paths.empty());
  assert(!par.best_paths.empty());
  assert(std::fabs(seq.best_length - par.best_length) < 1e-3);
}

int main() {
  TestGraphParsing();
  TestSequentialSolver();
  TestParallelSolverAgreement();
  std::cout << "PASSED" << std::endl;
  return 0;
}
