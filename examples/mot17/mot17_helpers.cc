#include "mot17_helpers.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <cppitertools/itertools.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include <cassert>

#if defined(__cpp_lib_filesystem) || __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __cpp_lib_experimental_filesystem || __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "no <filesystem> support"
#endif

using namespace percemon;
namespace ds = percemon::datastream;

namespace {

class CSVRow {
 public:
  std::string_view operator[](std::size_t index) const {
    return std::string_view(
        &m_line[m_data[index] + 1], m_data[index + 1] - (m_data[index] + 1));
  }
  size_t size() const { return m_data.size() - 1; }
  void readNextRow(std::istream& str) {
    std::getline(str, m_line);

    m_data.clear();
    m_data.emplace_back(-1);
    std::string::size_type pos = 0;
    while ((pos = m_line.find(',', pos)) != std::string::npos) {
      m_data.emplace_back(pos);
      ++pos;
    }
    // This checks for a trailing comma with no data after it.
    pos = m_line.size();
    m_data.emplace_back(pos);
  }

 private:
  std::string m_line;
  std::vector<int> m_data;
};

std::istream& operator>>(std::istream& str, CSVRow& data) {
  data.readNextRow(str);
  return str;
}

class CSVIterator {
 public:
  typedef std::input_iterator_tag iterator_category;
  typedef CSVRow value_type;
  typedef std::size_t difference_type;
  typedef CSVRow* pointer;
  typedef CSVRow& reference;

  CSVIterator(std::istream& str) : m_str(str.good() ? &str : NULL) { ++(*this); }
  CSVIterator() : m_str(NULL) {}

  // Pre Increment
  CSVIterator& operator++() {
    if (m_str) {
      if (!((*m_str) >> m_row)) { m_str = NULL; }
    }
    return *this;
  }
  // Post increment
  CSVIterator operator++(int) {
    CSVIterator tmp(*this);
    ++(*this);
    return tmp;
  }
  CSVRow const& operator*() const { return m_row; }
  CSVRow const* operator->() const { return &m_row; }

  bool operator==(CSVIterator const& rhs) {
    return ((this == &rhs) || ((this->m_str == NULL) && (rhs.m_str == NULL)));
  }
  bool operator!=(CSVIterator const& rhs) { return !((*this) == rhs); }

 private:
  std::istream* m_str;
  CSVRow m_row;
};

class CSVRange {
  std::istream& stream;

 public:
  CSVRange(std::istream& str) : stream(str) {}
  CSVIterator begin() const { return CSVIterator{stream}; }
  CSVIterator end() const { return CSVIterator{}; }
};

/**
 * A row from the MOT17 CSV evaluation file.
 */
struct ResultsRow {
  size_t frame;
  std::string id;
  double bb_left, bb_top, bb_width, bb_height;
  double confidence;
  int label;
  double visibility = 1.0;

  friend std::ostream& operator<<(std::ostream& os, const ResultsRow& row) {
    auto str = fmt::format(
        "{} {{ {}, {}, {} }} @ ( {} {} {} {} )",
        row.frame,
        row.id,
        row.confidence,
        row.label,
        row.bb_left,
        row.bb_top,
        row.bb_width,
        row.bb_height);
    return os << str;
  }
};

} // namespace

std::vector<percemon::datastream::Frame> mot17::parse_results(
    const std::string& file,
    const double fps,
    const size_t frame_width,
    const size_t frame_height) {
  auto input_file_path = fs::path{file};
  if (!fs::exists(input_file_path)) {
    throw std::invalid_argument(
        fmt::format("Input CSV file {} does not exist.\n", file));
  }

  std::ifstream in_file{file};

  auto results  = std::vector<ResultsRow>{};
  size_t n_rows = 0;

  // Read each row into a results vector
  for (auto& row : CSVRange(in_file)) {
    size_t frame_num  = std::stoull(std::string{row[0]});
    std::string id    = std::string{row[1]};
    double bb_left    = std::stod(std::string{row[2]});
    double bb_top     = std::stod(std::string{row[3]});
    double bb_width   = std::stod(std::string{row[4]});
    double bb_height  = std::stod(std::string{row[5]});
    double confidence = std::stod(std::string{row[6]});
    int label = static_cast<int>(Labels::Pedestrian); // MOT17 is always pedestrian

    results.push_back(
        ResultsRow{
            frame_num, id, bb_left, bb_top, bb_width, bb_height, confidence, label});
    n_rows += 1;
  }

  assert(n_rows == results.size());

  // Now create a datastream.
  const size_t last_frame = std::prev(results.end())->frame;
  auto stream             = std::vector<ds::Frame>{};
  for (auto i : iter::range(last_frame)) {
    auto frame = ds::Frame{fps * i, i + 1, frame_width, frame_height, {}};
    stream.push_back(frame);
  }

  for (auto&& result : results) {
    auto id           = result.id;
    auto object_class = result.label;
    auto confidence   = result.confidence;
    auto bbox         = ds::BoundingBox{
        static_cast<size_t>(result.bb_left),
        static_cast<size_t>(result.bb_left + result.bb_width),
        static_cast<size_t>(result.bb_top),
        static_cast<size_t>(result.bb_top + result.bb_height)};
    std::map<std::string, ds::Object>& obj_map = stream.at(result.frame - 1).objects;
    obj_map[id] = ds::Object{object_class, confidence, bbox};
  }

  return stream;
}
