#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ant_colony_solver.h"
#include "graph.h"

namespace {

struct Options {
  std::vector<size_t> sizes = {3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500, 7000};
  size_t runs = 100;
  std::string output = "benchmark_results.csv";
  size_t ants = 128;
  size_t iterations = 150;
  double alpha = 1.0;
  double beta = 3.0;
  double evaporation = 0.5;
  double q = 100.0;
  unsigned int seed = 42;
  size_t max_out_degree = 15;
};

std::vector<std::string> Split(std::string_view text, char delimiter) {
  std::vector<std::string> result;
  std::string current;
  for (char ch : text) {
    if (ch == delimiter) {
      if (!current.empty()) {
        result.push_back(current);
        current.clear();
      }
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    result.push_back(current);
  }
  return result;
}

Options ParseArgs(int argc, char** argv) {
  Options options;
  std::map<std::string, std::string> kv;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    size_t eq = arg.find('=');
    if (eq == std::string::npos) {
      kv[arg] = "true";
    } else {
      kv[arg.substr(0, eq)] = arg.substr(eq + 1);
    }
  }

  auto get = [&](const std::string& key) -> std::optional<std::string> {
    auto it = kv.find(key);
    if (it == kv.end()) {
      return std::nullopt;
    }
    return it->second;
  };

  if (auto value = get("--sizes")) {
    std::vector<std::string> tokens = Split(*value, ',');
    std::vector<size_t> parsed;
    for (const std::string& token : tokens) {
      if (!token.empty()) {
        parsed.push_back(static_cast<size_t>(std::stoull(token)));
      }
    }
    if (!parsed.empty()) {
      options.sizes = std::move(parsed);
    }
  }
  if (auto value = get("--runs")) {
    options.runs = static_cast<size_t>(std::stoull(*value));
    if (options.runs == 0) {
      options.runs = 1;
    }
  }
  if (auto value = get("--output")) {
    options.output = *value;
  }
  if (auto value = get("--ants")) {
    options.ants = static_cast<size_t>(std::stoull(*value));
    if (options.ants == 0) {
      options.ants = 1;
    }
  }
  if (auto value = get("--iterations")) {
    options.iterations = static_cast<size_t>(std::stoull(*value));
    if (options.iterations == 0) {
      options.iterations = 1;
    }
  }
  if (auto value = get("--alpha")) {
    options.alpha = std::stod(*value);
  }
  if (auto value = get("--beta")) {
    options.beta = std::stod(*value);
  }
  if (auto value = get("--evaporation")) {
    options.evaporation = std::stod(*value);
  }
  if (auto value = get("--q")) {
    options.q = std::stod(*value);
  }
  if (auto value = get("--seed")) {
    options.seed = static_cast<unsigned int>(std::stoul(*value));
  }
  if (auto value = get("--max-out-degree")) {
    options.max_out_degree = static_cast<size_t>(std::stoull(*value));
    if (options.max_out_degree < 1) {
      options.max_out_degree = 1;
    }
  }

  return options;
}

std::string GenerateGraphviz(size_t vertices,
                             unsigned int seed,
                             size_t max_out_degree) {
  if (vertices < 2) {
    throw std::invalid_argument("Graph must have at least two vertices");
  }
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> weight_dist(1.0, 100.0);
  std::uniform_int_distribution<size_t> extra_dist(0, max_out_degree > 0 ? max_out_degree - 1 : 0);
  std::uniform_int_distribution<size_t> vertex_dist(0, vertices - 1);

  std::vector<std::unordered_map<size_t, double>> adjacency(vertices);

  auto add_edge = [&](size_t from, size_t to) {
    if (from == to) {
      return;
    }
    double weight = weight_dist(rng);
    adjacency[from].emplace(to, weight);
  };

  // Ensure at least one Hamiltonian cycle exists.
  for (size_t i = 0; i < vertices; ++i) {
    size_t next = (i + 1) % vertices;
    add_edge(i, next);
  }

  for (size_t i = 0; i < vertices; ++i) {
    size_t desired_out_degree = 1;
    if (max_out_degree > 1) {
      desired_out_degree += extra_dist(rng);
      desired_out_degree = std::min(desired_out_degree, max_out_degree);
    }
    while (adjacency[i].size() < desired_out_degree) {
      size_t candidate = vertex_dist(rng);
      if (candidate == i) {
        continue;
      }
      if (adjacency[i].find(candidate) != adjacency[i].end()) {
        continue;
      }
      add_edge(i, candidate);
    }
  }

  std::ostringstream oss;
  oss << "digraph G {\n";
  oss << std::fixed << std::setprecision(6);
  for (size_t i = 0; i < vertices; ++i) {
    oss << "  v" << i << ";\n";
  }
  for (size_t from = 0; from < vertices; ++from) {
    for (const auto& [to, weight] : adjacency[from]) {
      oss << "  v" << from << " -> v" << to << " [weight=" << weight << "];\n";
    }
  }
  oss << "}\n";
  return oss.str();
}

lr4::Graph BuildGraph(size_t vertices,
                      unsigned int seed,
                      size_t max_out_degree) {
  std::string graphviz = GenerateGraphviz(vertices, seed, max_out_degree);
  std::istringstream input(graphviz);
  return lr4::Graph::FromGraphviz(input);
}

struct Measurement {
  size_t vertices = 0;
  std::string variant;
  size_t threads = 0;
  double average_ms = 0.0;
};

double RunSequential(const lr4::AntColonySolver& solver,
                     const lr4::AntColonyParameters& base_params,
                     size_t runs) {
  double total = 0.0;
  for (size_t run = 0; run < runs; ++run) {
    lr4::AntColonyParameters params = base_params;
    params.seed += static_cast<unsigned int>(run);
    lr4::TourResult result = solver.RunSequential(params);
    total += result.elapsed_ms;
  }
  return total / static_cast<double>(runs);
}

double RunParallel(const lr4::AntColonySolver& solver,
                   const lr4::AntColonyParameters& base_params,
                   size_t runs,
                   size_t threads) {
  double total = 0.0;
  for (size_t run = 0; run < runs; ++run) {
    lr4::AntColonyParameters params = base_params;
    params.seed += static_cast<unsigned int>(run);
    lr4::TourResult result = solver.RunParallel(params, threads);
    total += result.elapsed_ms;
  }
  return total / static_cast<double>(runs);
}

std::vector<size_t> DetermineThreadCounts() {
  size_t hardware_threads = std::max<size_t>(1, std::thread::hardware_concurrency());
  std::vector<size_t> thread_counts = {1, 2, 4, hardware_threads * 8};
  thread_counts.erase(std::remove_if(thread_counts.begin(), thread_counts.end(),
                                     [](size_t value) { return value == 0; }),
                      thread_counts.end());
  std::sort(thread_counts.begin(), thread_counts.end());
  thread_counts.erase(std::unique(thread_counts.begin(), thread_counts.end()), thread_counts.end());
  return thread_counts;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    Options options = ParseArgs(argc, argv);
    std::vector<size_t> thread_counts = DetermineThreadCounts();

    std::vector<Measurement> results;
    results.reserve(options.sizes.size() * (thread_counts.size() + 1));

    std::cout << "Аппаратных логических ядер: "
              << std::max<unsigned int>(1, std::thread::hardware_concurrency()) << "\n";
    std::cout << "Будут использованы рабочие потоки: ";
    for (size_t i = 0; i < thread_counts.size(); ++i) {
      if (i != 0) {
        std::cout << ", ";
      }
      std::cout << thread_counts[i];
    }
    std::cout << "\n\n";

    for (size_t index = 0; index < options.sizes.size(); ++index) {
      size_t vertices = options.sizes[index];
      unsigned int graph_seed = options.seed + static_cast<unsigned int>(index * 9973);
      std::cout << "Готовим граф на " << vertices << " вершинах..." << std::endl;
      lr4::Graph graph = BuildGraph(vertices, graph_seed, options.max_out_degree);
      lr4::AntColonySolver solver(graph);

      lr4::AntColonyParameters params;
      params.ants = options.ants;
      params.iterations = options.iterations;
      params.alpha = options.alpha;
      params.beta = options.beta;
      params.evaporation = options.evaporation;
      params.q = options.q;
      params.seed = options.seed;

      std::cout << "  Последовательные запуски..." << std::flush;
      double seq_avg = RunSequential(solver, params, options.runs);
      std::cout << " среднее время " << std::setprecision(4) << seq_avg << " мс" << std::endl;
      results.push_back(Measurement{vertices, "sequential", 1, seq_avg});

      for (size_t threads : thread_counts) {
        std::cout << "  Параллельные запуски (" << threads << " потоков)..." << std::flush;
        double par_avg = RunParallel(solver, params, options.runs, threads);
        std::cout << " среднее время " << std::setprecision(4) << par_avg << " мс" << std::endl;
        results.push_back(Measurement{vertices, "parallel", threads, par_avg});
      }

      std::cout << std::endl;
    }

    std::ofstream csv(options.output);
    if (!csv) {
      throw std::runtime_error("Unable to open output file: " + options.output);
    }
    csv << "vertices,variant,threads,average_ms\n";
    csv << std::fixed << std::setprecision(6);
    for (const Measurement& measurement : results) {
      csv << measurement.vertices << ','
          << measurement.variant << ','
          << measurement.threads << ','
          << measurement.average_ms << "\n";
    }

    std::cout << "Результаты сохранены в " << options.output << std::endl;
    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::cerr << "Ошибка: " << ex.what() << std::endl;
  }
  return EXIT_FAILURE;
}
