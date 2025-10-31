#include "ant_colony_solver.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace lr4 {
namespace {

double Heuristic(double weight) {
  if (weight <= 0.0 || std::isinf(weight)) {
    return 0.0;
  }
  return 1.0 / weight;
}

bool AreEqual(double a, double b) {
  constexpr double kEps = 1e-9;
  return std::fabs(a - b) <= kEps;
}

}  // namespace

AntColonySolver::AntColonySolver(const Graph& graph) : graph_(graph) {}

TourResult AntColonySolver::RunSequential(const AntColonyParameters& params) const {
  TourResult result;
  auto pheromone = InitialPheromone();
  std::mt19937 rng(params.seed);
  auto start = std::chrono::steady_clock::now();
  for (size_t iteration = 0; iteration < params.iterations; ++iteration) {
    std::vector<std::vector<double>> delta(graph_.VertexCount(),
                                           std::vector<double>(graph_.VertexCount(), 0.0));
    for (size_t ant = 0; ant < params.ants; ++ant) {
      AntPath path = ConstructSolution(rng, params, pheromone);
      if (path.path.empty()) {
        continue;
      }
      DepositPheromone(path, params.q, &delta);
      UpdateBest(path, &result.best_paths, &result.best_length, &result.best_paths_labels);
    }
    for (size_t i = 0; i < graph_.VertexCount(); ++i) {
      for (size_t j = 0; j < graph_.VertexCount(); ++j) {
        pheromone[i][j] = (1.0 - params.evaporation) * pheromone[i][j] + delta[i][j];
        if (pheromone[i][j] < 1e-12) {
          pheromone[i][j] = 1e-12;
        }
      }
    }
  }
  auto end = std::chrono::steady_clock::now();
  result.elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
  return result;
}

TourResult AntColonySolver::RunParallel(const AntColonyParameters& params,
                                        size_t thread_count) const {
  TourResult result;
  if (thread_count == 0) {
    return result;
  }
  auto pheromone = InitialPheromone();
  auto start = std::chrono::steady_clock::now();
  std::mutex best_mutex;
  for (size_t iteration = 0; iteration < params.iterations; ++iteration) {
    std::vector<std::vector<double>> delta(graph_.VertexCount(),
                                           std::vector<double>(graph_.VertexCount(), 0.0));
    std::vector<std::vector<std::vector<double>>> local_deltas(thread_count,
        std::vector<std::vector<double>>(graph_.VertexCount(), std::vector<double>(graph_.VertexCount(), 0.0)));
    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    size_t base = params.ants / thread_count;
    size_t remainder = params.ants % thread_count;
    for (size_t t = 0; t < thread_count; ++t) {
      size_t assigned = base + (t < remainder ? 1 : 0);
      workers.emplace_back([&, t, assigned]() {
        if (assigned == 0) {
          return;
        }
        std::mt19937 rng(params.seed + static_cast<unsigned int>(t * 9973 + iteration * 7919));
        double thread_best_length = Graph::kInfinity;
        std::vector<AntPath> thread_best_paths;
        for (size_t ant = 0; ant < assigned; ++ant) {
          AntPath path = ConstructSolution(rng, params, pheromone);
          if (path.path.empty()) {
            continue;
          }
          DepositPheromone(path, params.q, &local_deltas[t]);
          if (path.length + 1e-9 < thread_best_length) {
            thread_best_length = path.length;
            thread_best_paths.clear();
            thread_best_paths.push_back(path);
          } else if (AreEqual(path.length, thread_best_length)) {
            thread_best_paths.push_back(path);
          }
        }
        if (!thread_best_paths.empty()) {
          std::lock_guard<std::mutex> lock(best_mutex);
          for (const AntPath& best_local : thread_best_paths) {
            UpdateBest(best_local, &result.best_paths, &result.best_length, &result.best_paths_labels);
          }
        }
      });
    }
    for (auto& worker : workers) {
      worker.join();
    }
    for (size_t t = 0; t < thread_count; ++t) {
      for (size_t i = 0; i < graph_.VertexCount(); ++i) {
        for (size_t j = 0; j < graph_.VertexCount(); ++j) {
          delta[i][j] += local_deltas[t][i][j];
        }
      }
    }
    for (size_t i = 0; i < graph_.VertexCount(); ++i) {
      for (size_t j = 0; j < graph_.VertexCount(); ++j) {
        pheromone[i][j] = (1.0 - params.evaporation) * pheromone[i][j] + delta[i][j];
        if (pheromone[i][j] < 1e-12) {
          pheromone[i][j] = 1e-12;
        }
      }
    }
  }
  auto end = std::chrono::steady_clock::now();
  result.elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
  return result;
}

AntColonySolver::AntPath AntColonySolver::ConstructSolution(
    std::mt19937& rng,
    const AntColonyParameters& params,
    const std::vector<std::vector<double>>& pheromone) const {
  AntPath path;
  const size_t n = graph_.VertexCount();
  if (n == 0) {
    return path;
  }
  std::uniform_int_distribution<size_t> start_dist(0, n - 1);
  size_t current = start_dist(rng);
  std::vector<int> visited(n, 0);
  visited[current] = 1;
  path.path.push_back(static_cast<int>(current));
  for (size_t step = 1; step < n; ++step) {
    std::vector<double> probabilities;
    std::vector<size_t> candidates;
    probabilities.reserve(n);
    candidates.reserve(n);
    double sum = 0.0;
    for (size_t next = 0; next < n; ++next) {
      if (visited[next]) {
        continue;
      }
      double weight = graph_.Weight(current, next);
      double tau = std::pow(pheromone[current][next], params.alpha);
      double eta = std::pow(Heuristic(weight), params.beta);
      double value = tau * eta;
      if (value <= 0.0) {
        continue;
      }
      candidates.push_back(next);
      probabilities.push_back(value);
      sum += value;
    }
    if (candidates.empty()) {
      path.path.clear();
      path.length = Graph::kInfinity;
      return path;
    }
    std::uniform_real_distribution<double> dist(0.0, sum);
    double choice = dist(rng);
    size_t index = 0;
    double cumulative = probabilities[0];
    while (choice > cumulative && index + 1 < probabilities.size()) {
      ++index;
      cumulative += probabilities[index];
    }
    current = candidates[index];
    visited[current] = 1;
    path.path.push_back(static_cast<int>(current));
  }
  path.path.push_back(path.path.front());
  path.length = ComputePathLength(path.path);
  if (!std::isfinite(path.length)) {
    path.path.clear();
    path.length = Graph::kInfinity;
  }
  return path;
}

void AntColonySolver::DepositPheromone(const AntPath& path,
                                       double q,
                                       std::vector<std::vector<double>>* deltas) {
  if (path.path.size() < 2 || !std::isfinite(path.length)) {
    return;
  }
  const double deposit = q / path.length;
  for (size_t i = 0; i + 1 < path.path.size(); ++i) {
    const int from = path.path[i];
    const int to = path.path[i + 1];
    (*deltas)[static_cast<size_t>(from)][static_cast<size_t>(to)] += deposit;
  }
}

void AntColonySolver::UpdateBest(const AntPath& candidate,
                                 std::vector<std::vector<int>>* best_paths,
                                 double* best_length,
                                 std::vector<std::string>* best_labels) const {
  if (candidate.path.empty() || !std::isfinite(candidate.length)) {
    return;
  }
  std::vector<int> canonical = graph_.CanonicalizeTour(candidate.path);
  std::string serialized;
  for (size_t i = 0; i < canonical.size(); ++i) {
    if (i != 0) {
      serialized += "->";
    }
    serialized += graph_.Label(static_cast<size_t>(canonical[i]));
  }
  if (best_paths->empty() || candidate.length + 1e-9 < *best_length) {
    *best_length = candidate.length;
    best_paths->clear();
    best_labels->clear();
    best_paths->push_back(std::move(canonical));
    best_labels->push_back(serialized);
  } else if (AreEqual(candidate.length, *best_length)) {
    if (std::find(best_labels->begin(), best_labels->end(), serialized) == best_labels->end()) {
      best_paths->push_back(std::move(canonical));
      best_labels->push_back(serialized);
    }
  }
}

std::vector<std::vector<double>> AntColonySolver::InitialPheromone() const {
  const size_t n = graph_.VertexCount();
  const double initial_value = 1.0;
  return std::vector<std::vector<double>>(n, std::vector<double>(n, initial_value));
}

double AntColonySolver::ComputePathLength(const std::vector<int>& path) const {
  if (path.size() < 2) {
    return Graph::kInfinity;
  }
  double length = 0.0;
  for (size_t i = 0; i + 1 < path.size(); ++i) {
    double weight = graph_.Weight(static_cast<size_t>(path[i]), static_cast<size_t>(path[i + 1]));
    if (!std::isfinite(weight)) {
      return Graph::kInfinity;
    }
    length += weight;
  }
  return length;
}

std::vector<std::string> AntColonySolver::PathToLabels(const std::vector<int>& path) const {
  std::vector<std::string> labels;
  labels.reserve(path.size());
  for (int index : path) {
    labels.push_back(graph_.Label(static_cast<size_t>(index)));
  }
  return labels;
}

}  // namespace lr4
