#include <regex>
#include <argparse/argparse.hpp>
#include <fstream>
#include <sstream>
#include <string>

std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("ecscanner");

  program.add_argument("filename")
    .help("input file")
    .metavar("FILENAME");

  try {
    program.parse_args(argc, argv);
  }
  catch (const std::exception& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  auto input = program.get<std::string>("filename");
  auto file_str = readFile(input);
  std::cout << file_str << std::endl;

  return 0;
}
