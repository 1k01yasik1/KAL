#include "graph.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace lr4 {
namespace {

std::string Trim(std::string_view text) {
  size_t start = 0;
  while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
    ++start;
  }
  size_t end = text.size();
  while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string{text.substr(start, end - start)};
}

std::string StripQuotes(std::string_view text) {
  if (text.size() >= 2 && ((text.front() == '"' && text.back() == '"') ||
                           (text.front() == '\'' && text.back() == '\''))) {
    return std::string{text.substr(1, text.size() - 2)};
  }
  return std::string{text};
}

struct RawEdge {
  std::string from;
  std::string to;
  double weight;
  bool bidirectional;
};

std::optional<double> ParseWeight(std::string_view attributes) {
  static const std::regex kWeightRegex(R"((weight|label|w)\s*=\s*([-+]?([0-9]*\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)?))");
  std::cmatch match;
  std::string attr(attributes);
  if (std::regex_search(attr.c_str(), match, kWeightRegex)) {
    return std::stod(match[2]);
  }
  static const std::regex kNumberOnly(R"([-+]?([0-9]*\.[0-9]+|[0-9]+)([eE][-+]?[0-9]+)?)");
  if (std::regex_search(attr.c_str(), match, kNumberOnly)) {
    return std::stod(match[0]);
  }
  return std::nullopt;
}

RawEdge ParseEdgeLine(const std::string& line) {
  std::string trimmed = Trim(line);
  if (trimmed.empty() || trimmed[0] == '#') {
    return RawEdge{"", "", 1.0, false};
  }

  size_t arrow_pos = trimmed.find("->");
  bool bidirectional = false;
  size_t operator_length = 2;
  if (arrow_pos == std::string::npos) {
    arrow_pos = trimmed.find("--");
    bidirectional = true;
  }
  if (arrow_pos == std::string::npos) {
    return RawEdge{"", "", 1.0, false};
  }
  std::string from_token = Trim(trimmed.substr(0, arrow_pos));
  std::string rest = trimmed.substr(arrow_pos + operator_length);
  size_t bracket_pos = rest.find('[');
  std::string to_token = rest;
  std::string attributes;
  if (bracket_pos != std::string::npos) {
    to_token = Trim(rest.substr(0, bracket_pos));
    attributes = rest.substr(bracket_pos);
  }
  size_t semicolon_pos = to_token.find(';');
  if (semicolon_pos != std::string::npos) {
    to_token = to_token.substr(0, semicolon_pos);
  }
  to_token = Trim(to_token);

  if (from_token.empty() || to_token.empty()) {
    return RawEdge{"", "", 1.0, false};
  }

  RawEdge edge;
  edge.from = StripQuotes(from_token);
  edge.to = StripQuotes(to_token);
  edge.bidirectional = bidirectional;
  edge.weight = 1.0;
  if (!attributes.empty()) {
    if (auto weight = ParseWeight(attributes)) {
      edge.weight = *weight;
    }
  }
  return edge;
}

}  // namespace

Graph Graph::FromGraphvizFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Unable to open graph file: " + path);
  }
  return FromGraphviz(input);
}

Graph Graph::FromGraphviz(std::istream& input) {
  std::vector<RawEdge> edges;
  std::unordered_set<std::string> labels;
  std::string line;
  while (std::getline(input, line)) {
    RawEdge edge = ParseEdgeLine(line);
    if (edge.from.empty() || edge.to.empty()) {
      continue;
    }
    labels.insert(edge.from);
    labels.insert(edge.to);
    edges.push_back(edge);
  }
  if (labels.empty()) {
    return Graph{};
  }
  std::vector<std::string> sorted_labels(labels.begin(), labels.end());
  std::sort(sorted_labels.begin(), sorted_labels.end());
  Graph graph;
  graph.index_to_label_ = sorted_labels;
  for (size_t i = 0; i < sorted_labels.size(); ++i) {
    graph.label_to_index_.emplace(sorted_labels[i], i);
  }
  const size_t n = sorted_labels.size();
  graph.adjacency_.assign(n, std::vector<double>(n, kInfinity));
  for (size_t i = 0; i < n; ++i) {
    graph.adjacency_[i][i] = 0.0;
  }
  for (const RawEdge& edge : edges) {
    auto from_it = graph.label_to_index_.find(edge.from);
    auto to_it = graph.label_to_index_.find(edge.to);
    if (from_it == graph.label_to_index_.end() || to_it == graph.label_to_index_.end()) {
      continue;
    }
    graph.adjacency_[from_it->second][to_it->second] = edge.weight;
    if (edge.bidirectional) {
      graph.adjacency_[to_it->second][from_it->second] = edge.weight;
    }
  }
  return graph;
}

std::vector<int> Graph::CanonicalizeTour(const std::vector<int>& tour) const {
  if (tour.size() <= 1) {
    return tour;
  }
  std::vector<int> cycle = tour;
  if (cycle.front() == cycle.back()) {
    cycle.pop_back();
  }
  if (cycle.empty()) {
    return tour;
  }
  const size_t n = cycle.size();
  auto build_key = [this, &cycle, n](size_t start, bool reverse) {
    std::string key;
    key.reserve(n * 4);
    auto append = [this, &key](int index) {
      if (!key.empty()) {
        key.push_back('>');
      }
      key += Label(static_cast<size_t>(index));
    };
    if (!reverse) {
      for (size_t i = 0; i < n; ++i) {
        append(cycle[(start + i) % n]);
      }
    } else {
      size_t index = start % n;
      for (size_t i = 0; i < n; ++i) {
        append(cycle[index]);
        if (index == 0) {
          index = n - 1;
        } else {
          --index;
        }
      }
    }
    return key;
  };
  size_t best_shift = 0;
  bool best_reverse = false;
  std::string best_key = build_key(0, false);
  for (size_t shift = 0; shift < n; ++shift) {
    std::string forward_key = build_key(shift, false);
    if (forward_key < best_key) {
      best_key = forward_key;
      best_shift = shift;
      best_reverse = false;
    }
    std::string reverse_key = build_key(shift, true);
    if (reverse_key < best_key) {
      best_key = reverse_key;
      best_shift = shift;
      best_reverse = true;
    }
  }
  std::vector<int> result;
  result.reserve(n + 1);
  if (!best_reverse) {
    for (size_t i = 0; i < n; ++i) {
      result.push_back(cycle[(best_shift + i) % n]);
    }
  } else {
    size_t index = best_shift % n;
    for (size_t i = 0; i < n; ++i) {
      result.push_back(cycle[index]);
      if (index == 0) {
        index = n - 1;
      } else {
        --index;
      }
    }
  }
  result.push_back(result.front());
  return result;
}

}  // namespace lr4
