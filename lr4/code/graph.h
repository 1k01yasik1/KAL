#ifndef LR4_GRAPH_H
#define LR4_GRAPH_H

#include <cstddef>
#include <istream>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lr4 {

class Graph {
 public:
  static Graph FromGraphvizFile(const std::string& path);
  static Graph FromGraphviz(std::istream& input);

  size_t VertexCount() const { return index_to_label_.size(); }
  double Weight(size_t from, size_t to) const {
    return adjacency_[from][to];
  }
  const std::string& Label(size_t index) const { return index_to_label_[index]; }

  std::vector<int> CanonicalizeTour(const std::vector<int>& tour) const;

  static constexpr double kInfinity = std::numeric_limits<double>::infinity();

 private:
  std::vector<std::string> index_to_label_;
  std::unordered_map<std::string, size_t> label_to_index_;
  std::vector<std::vector<double>> adjacency_;
};

}  // namespace lr4

#endif  // LR4_GRAPH_H
