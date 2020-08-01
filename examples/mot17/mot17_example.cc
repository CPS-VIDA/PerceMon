#include "mot17_helpers.hpp"

#include "percemon/fmt.hpp"
#include "percemon/percemon.hpp"

#include <CLI/CLI.hpp>
#include <itertools.hpp>
#include <spdlog/spdlog.h>

namespace ds = percemon::datastream;

percemon::Expr get_phi() {
  using namespace percemon;

  // There exists at least two unique objects from the same class
  auto id1 = Var_id{"1"};
  auto id2 = Var_id{"2"};
  Expr phi = Exists({id1, id2})->dot(Expr{(id1 == id2)} & Class(id1) == Class(id2));

  return phi;
}

void compute(
    percemon::monitoring::OnlineMonitor& monitor,
    const std::vector<ds::Frame>& trace) {
  spdlog::info("Beginning compute.\n");

  for (auto&& [i, frame] : iter::enumerate(trace)) {
    monitor.add_frame(frame);
    double rob = monitor.eval();
    spdlog::info("rob[{}]\t= {}\n", i, rob);
  }

  spdlog::info("End of compute.\n");
}

int main(int argc, char* argv[]) {
  CLI::App app{
      "Example demonstrating PerceMon tracking specifications on MOT17 results"};

  std::string filename;
  app.add_option("input", filename, "Input MOT17 results file")->required();

  double fps;
  app.add_option("-f,--fps", fps, "FPS for given stream.")->required();
  std::pair<size_t, size_t> size;
  app.add_option("--size", size, "Width X Height of images in stream.")->required();

  CLI11_PARSE(app, argc, argv);

  size_t width  = size.first;
  size_t height = size.second;

  auto phi     = get_phi();
  auto trace   = mot17::parse_results(filename, fps, width, height);
  auto monitor = percemon::monitoring::OnlineMonitor{phi, fps};
  compute(monitor, trace);

  return 0;
}
