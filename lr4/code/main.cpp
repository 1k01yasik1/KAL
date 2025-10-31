#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <thread>

#include "ant_colony_solver.h"
#include "graph.h"

namespace {

struct Options {
  std::string graph_path = "code/data/sample.dot";
  size_t ants = 128;
  size_t iterations = 150;
  size_t threads = std::max<size_t>(1, std::thread::hardware_concurrency());
  bool only_sequential = false;
  bool only_parallel = false;
bool print_paths = true;
  unsigned int seed = 42;
};

Options ParseArgs(int argc, char** argv) {
  Options options;
  std::map<std::string, std::string> kv;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    size_t eq_pos = arg.find('=');
    if (eq_pos != std::string::npos) {
      kv[arg.substr(0, eq_pos)] = arg.substr(eq_pos + 1);
    } else {
      kv[arg] = "true";
    }
  }
  auto get = [&](const std::string& key) -> std::optional<std::string> {
    auto it = kv.find(key);
    if (it == kv.end()) {
      return std::nullopt;
    }
    return it->second;
  };
  if (auto value = get("--graph")) {
    options.graph_path = *value;
  }
  if (auto value = get("--ants")) {
    options.ants = static_cast<size_t>(std::stoul(*value));
  }
  if (auto value = get("--iterations")) {
    options.iterations = static_cast<size_t>(std::stoul(*value));
  }
  if (auto value = get("--threads")) {
    options.threads = static_cast<size_t>(std::stoul(*value));
    if (options.threads == 0) {
      options.threads = 1;
    }
  }
  if (auto value = get("--seed")) {
    options.seed = static_cast<unsigned int>(std::stoul(*value));
  }
  if (auto value = get("--only-seq")) {
    options.only_sequential = value == std::nullopt || *value == "true";
  }
  if (auto value = get("--only-par")) {
    options.only_parallel = value == std::nullopt || *value == "true";
  }
  if (auto value = get("--print-paths")) {
    options.print_paths = value == std::nullopt || *value != "false";
  }
  return options;
}

void PrintResult(const std::string& title,
                 const lr4::TourResult& result,
                 const lr4::Graph& graph,
                 bool print_paths) {
  std::cout << "== " << title << " ==\n";
  if (!std::isfinite(result.best_length)) {
    std::cout << "Не удалось построить допустимый цикл." << std::endl;
    return;
  }
  std::cout << "Лучший найденный путь длины: " << std::fixed << std::setprecision(3)
            << result.best_length << "\n";
  std::cout << "Количество маршрутов с оптимальной длиной: " << result.best_paths.size() << "\n";
  std::cout << "Время выполнения: " << std::setprecision(2) << result.elapsed_ms << " мс\n";
  if (print_paths) {
    for (size_t i = 0; i < result.best_paths.size(); ++i) {
      std::cout << "Маршрут " << (i + 1) << ": ";
      const auto& path = result.best_paths[i];
      for (size_t j = 0; j < path.size(); ++j) {
        if (j != 0) {
          std::cout << " -> ";
        }
        std::cout << graph.Label(static_cast<size_t>(path[j]));
      }
      std::cout << "\n";
    }
  }
  std::cout << std::endl;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    Options options = ParseArgs(argc, argv);
    lr4::Graph graph = lr4::Graph::FromGraphvizFile(options.graph_path);
    lr4::AntColonySolver solver(graph);
    lr4::AntColonyParameters params;
    params.ants = options.ants;
    params.iterations = options.iterations;
    params.seed = options.seed;
    std::cout << "Граф содержит вершин: " << graph.VertexCount() << "\n";
    std::cout << "Настройки: муравьёв=" << params.ants << ", итераций=" << params.iterations
              << ", потоки=" << options.threads << "\n\n";
    if (!options.only_parallel) {
      lr4::TourResult seq = solver.RunSequential(params);
      PrintResult("Последовательный алгоритм", seq, graph, options.print_paths);
    }
    if (!options.only_sequential) {
      lr4::TourResult par = solver.RunParallel(params, options.threads);
      PrintResult("Параллельный алгоритм", par, graph, options.print_paths);
    }
    return EXIT_SUCCESS;
  } catch (const std::exception& ex) {
    std::cerr << "Ошибка: " << ex.what() << std::endl;
  }
  return EXIT_FAILURE;
}
