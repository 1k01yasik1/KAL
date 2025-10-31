#ifndef LR4_ANT_COLONY_SOLVER_H
#define LR4_ANT_COLONY_SOLVER_H

#include <cstddef>
#include <mutex>
#include <random>
#include <vector>

#include "graph.h"

namespace lr4 {

struct AntColonyParameters {
  size_t ants = 64;
  size_t iterations = 100;
  double alpha = 1.0;         // influence of pheromone
  double beta = 3.0;          // influence of heuristic (1 / distance)
  double evaporation = 0.5;   // pheromone evaporation rate
  double q = 100.0;           // pheromone deposit factor
  unsigned int seed = 42;     // random seed
};

struct TourResult {
  double best_length = Graph::kInfinity;
  std::vector<std::vector<int>> best_paths;
  std::vector<std::string> best_paths_labels;
  double elapsed_ms = 0.0;
};

class AntColonySolver {
 public:
  explicit AntColonySolver(const Graph& graph);

  TourResult RunSequential(const AntColonyParameters& params) const;
  TourResult RunParallel(const AntColonyParameters& params, size_t thread_count) const;

 private:
  struct AntPath {
    std::vector<int> path;
    double length = Graph::kInfinity;
  };

  AntPath ConstructSolution(std::mt19937& rng,
                            const AntColonyParameters& params,
                            const std::vector<std::vector<double>>& pheromone) const;

  static void DepositPheromone(const AntPath& path,
                               double q,
                               std::vector<std::vector<double>>* deltas);

  void UpdateBest(const AntPath& candidate,
                  std::vector<std::vector<int>>* best_paths,
                  double* best_length,
                  std::vector<std::string>* best_labels) const;

  std::vector<std::vector<double>> InitialPheromone() const;

  double ComputePathLength(const std::vector<int>& path) const;

  std::vector<std::string> PathToLabels(const std::vector<int>& path) const;

  const Graph& graph_;
};

}  // namespace lr4

#endif  // LR4_ANT_COLONY_SOLVER_H
