#include "prp.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <chrono>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s file\n", argv[0]);
    exit(1);
  }

  Prp scanner;
  auto start = std::chrono::system_clock::now();
  scanner.parse_file(argv[1]);
  auto end = std::chrono::system_clock::now();

  auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::cout << "Elapsed time: " << elapsed_time.count() << " ms\n";

  return 0;
}
