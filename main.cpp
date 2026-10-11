

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct StationTemperature {
  double min;
  double sum;
  double max;
  size_t count;
};

// 1brc using the simple naive approach
// using std::ifstream for reading.
// a hashmap for keep track of the aggregates and a vector for sorting
inline void FirstApproach() {
  std::ifstream file{"measurements.txt"};

  if (!file.is_open()) {
    std::cout << "failed to open the file\n";
    return;
  }

  std::unordered_map<std::string, StationTemperature> station_map;
  std::vector<std::string> stations;

  std::string line;
  while (std::getline(file, line)) {
    size_t pos = line.find(';');
    assert(pos != std::string::npos);

    auto station_name = line.substr(0, pos);
    auto temp = std::stod(line.substr(pos + 1));

    auto it = station_map.find(station_name);
    if (it == station_map.end()) {
      station_map.emplace(station_name,
                          StationTemperature{temp, temp, temp, 1});
      stations.push_back(station_name);
    } else {
      auto &resting_temp = it->second;
      resting_temp.min = std::min(resting_temp.min, temp);
      resting_temp.max = std::max(resting_temp.max, temp);
      resting_temp.sum += temp;
      ++resting_temp.count;
    }
  }

  std::sort(stations.begin(), stations.end());

  std::cout << "{";
  bool first = true;
  for (const auto &name : stations) {
    const auto &stats = station_map.at(name);
    double mean = stats.sum / stats.count;

    if (!first)
      std::cout << ", ";
    first = false;

    std::cout << name << "=" << std::fixed << std::setprecision(1) << stats.min
              << "/" << mean << "/" << stats.max;
  }
  std::cout << "}\n";
}

// Same as above except we mmap a region.
// This reduces an additional copy from kernel-space to user-space page
inline void SecondApproach() {
  int fd_{-1};
  void *mem{nullptr};

  fd_ = open("measurements.txt", O_RDONLY);
  if (fd_ == -1) {
    std::perror("Error opening file for reading\n");
    return;
  }

  struct stat st;
  if (fstat(fd_, &st) == -1) {
    close(fd_);
    std::perror("fstat failed\n");
    return;
  }

  size_t FILE_SIZE = st.st_size;

  mem = mmap(NULL, FILE_SIZE, PROT_READ, MAP_PRIVATE, fd_, 0);
  if (mem == MAP_FAILED) {
    close(fd_);
    std::perror("Error mmapping the file\n");
    return;
  }

  madvise(mem, FILE_SIZE, MADV_SEQUENTIAL);

  std::unordered_map<std::string, StationTemperature> station_map;
  std::vector<std::string> stations;

  const char *data = static_cast<const char *>(mem);
  const char *end = data + FILE_SIZE;
  const char *p = data;

  while (p < end) {
    const char *line_start = p;
    while (p < end && *p != '\n') {
      ++p;
    }
    // line is [line_start..p)

    const char *semi = line_start;
    while (semi < p && *semi != ';') {
      ++semi;
    }
    // station name is [line_start..semi)
    // temp is [semi+1..p)

    std::string station_name(line_start, semi);
    double temp = std::stod(std::string(semi + 1, p));

    auto it = station_map.find(station_name);
    if (it == station_map.end()) {
      station_map.emplace(station_name,
                          StationTemperature{temp, temp, temp, 1});
      stations.push_back(station_name);
    } else {
      auto &s = it->second;
      s.min = std::min(s.min, temp);
      s.max = std::max(s.max, temp);
      s.sum += temp;
      ++s.count;
    }

    if (p < end) {
      ++p;
    }
  }

  std::sort(stations.begin(), stations.end());

  std::cout << "{";
  bool first = true;
  for (const auto &name : stations) {
    const auto &stats = station_map.at(name);
    double mean = stats.sum / stats.count;

    if (!first)
      std::cout << ", ";
    first = false;

    std::cout << name << "=" << std::fixed << std::setprecision(1) << stats.min
              << "/" << mean << "/" << stats.max;
  }
  std::cout << "}\n";

  munmap(mem, FILE_SIZE);
  close(fd_);
  return;
}

int main() { SecondApproach(); }
