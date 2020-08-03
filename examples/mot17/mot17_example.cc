#include "mot17_helpers.hpp"

#include "percemon/fmt.hpp"
#include "percemon/percemon.hpp"

#include <CLI/CLI.hpp>
#include <itertools.hpp>
#include <spdlog/spdlog.h>

#if defined(__cpp_lib_filesystem) || __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __cpp_lib_experimental_filesystem || __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "no <filesystem> support"
#endif

namespace ds = percemon::datastream;

enum class PhiNumber : int { Example1, Example2, Example3 };

percemon::Expr get_phi1() {
  using namespace percemon;

  // There exists at least two unique objects from the same class
  auto id1 = Var_id{"1"};
  auto id2 = Var_id{"2"};
  Expr phi = Exists({id1, id2})->dot((id1 == id2) & Expr{Class(id1) == Class(id2)});

  return phi;
}

percemon::Expr get_phi2() {
  using namespace percemon;

  auto id1 = Var_id{"1"};
  auto id2 = Var_id{"2"};
  Expr phi = Forall({id1})->dot(
      Expr{Previous(Const{true})} >>
      Exists({id2})->dot((id1 == id2) & Expr{Class(id1) == Class(id2)}));

  return phi;
}

percemon::Expr get_phi3() {
  using namespace percemon;

  auto id1 = Var_id{"1"};
  auto id2 = Var_id{"2"};
  auto x   = Var_x{"1"};
  auto f   = Var_f{"1"};

  Expr phi1 = And({1 <= f - C_FRAME{}, f - C_FRAME{} <= 2});
  Expr phi2 = Exists({id2})->dot((id1 == id2) & Expr{Class(id1) == Class(id2)});

  Expr phi = Forall({id1})->at({x, f})->dot(Always(phi1 >> phi2));
  return phi;
}

percemon::Expr get_phi(PhiNumber opt) {
  switch (opt) {
    case PhiNumber::Example1: return get_phi1();
    case PhiNumber::Example2: return get_phi2();
    case PhiNumber::Example3: return get_phi3();
  }
  return {};
}

std::vector<bool> compute(
    percemon::monitoring::OnlineMonitor& monitor,
    const std::vector<ds::Frame>& trace) {
  spdlog::info("Running online monitor for:\n\t {}", monitor.phi);
  spdlog::info("Horizon:  {}", monitor.getHorizon());
  spdlog::info("FPS:      {}", monitor.fps);

  std::vector<bool> sat_unsat;

  for (auto&& [i, it] = std::make_tuple(0, std::begin(trace)); it != std::end(trace);
       it++, i++) {
    auto& frame = *it;
    monitor.add_frame(frame);
    double rob = monitor.eval();
    sat_unsat.push_back(rob >= 0);
    spdlog::debug("rob[{}]\t= {}", i, rob);
  }

  return sat_unsat;
}

void save_to_file(const std::vector<bool>& out, const std::string& file) {
  auto output_file_path = fs::path(file, std::ios_base::out);
  fs::create_directories(output_file_path.parent_path());

  std::ofstream out_file{output_file_path};
  for (auto&& [i, b] : iters::enumerate(out)) {
    out_file << fmt::format("{},{}", i, (b) ? 1 : 0);
  }
}

int main(int argc, char* argv[]) {
  CLI::App app{
      "Example demonstrating PerceMon tracking specifications on MOT17 results"};

  std::string filename;
  app.add_option("input", filename, "Input MOT17 results file")->required();

  std::string out_file = "robustness.csv";
  app.add_option("-o,--output", out_file, "Save frame by frame robustness results to?");

  double fps;
  app.add_option("-f,--fps", fps, "FPS for given stream.")->required();
  std::pair<size_t, size_t> size;
  app.add_option("--size", size, "Width X Height of images in stream.")->required();

  PhiNumber phi_option = PhiNumber::Example1;
  app.add_option("--phi", phi_option, "Which example to run?")
      ->transform(CLI::CheckedTransformer(
          std::map<std::string, PhiNumber>{{"phi1", PhiNumber::Example1},
                                           {"phi2", PhiNumber::Example2},
                                           {"phi3", PhiNumber::Example3}},
          CLI::ignore_case));

  bool verbose = false;
  app.add_flag("-v,--verbose", verbose, "If set, will set logging level to DEBUG");

  CLI11_PARSE(app, argc, argv);

  if (verbose) {
    spdlog::set_level(spdlog::level::debug); // Set global log level to debug
  }

  size_t width  = size.first;
  size_t height = size.second;

  percemon::Expr phi = get_phi(phi_option);
  auto trace         = mot17::parse_results(filename, fps, width, height);
  auto monitor       = percemon::monitoring::OnlineMonitor{phi, fps};
  auto frame_rob     = compute(monitor, trace);

  return 0;
}
