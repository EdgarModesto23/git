#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <zlib.h>

std::string getBlobData(const std::vector<Bytef> &decompressedData) {
  auto afterNull{false};
  std::string decompressed_content{};
  for (const auto &i : decompressedData) {
    if (!(afterNull)) {
      if (i == 0) {
        afterNull = true;
        continue;
      }
      continue;
    }
    decompressed_content.push_back(i);
  }
  return decompressed_content;
}

void decompress(const std::vector<Bytef> &compressedData,
                std::vector<Bytef> &decompressedData) {
  // Set up the zlib stream for decompression
  z_stream strm = {};
  strm.avail_in = compressedData.size();
  strm.next_in = const_cast<Bytef *>(compressedData.data());

  // Initialize the decompression stream
  if (inflateInit(&strm) != Z_OK) {
    throw std::runtime_error("Failed to initialize zlib decompression.");
  }

  // Set the output buffer size
  size_t bufferSize = compressedData.size() * 2; // A safe initial size
  decompressedData.resize(bufferSize);

  // Perform decompression
  strm.avail_out = bufferSize;
  strm.next_out = decompressedData.data();
  int ret = inflate(&strm, Z_NO_FLUSH);

  // Check for errors
  if (ret != Z_STREAM_END) {
    inflateEnd(&strm);
    throw std::runtime_error("Decompression failed.");
  }

  // Resize the output buffer to the actual decompressed size
  decompressedData.resize(bufferSize - strm.avail_out);

  // Clean up
  inflateEnd(&strm);
}

int main(int argc, char *argv[]) {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  if (argc < 2) {
    std::cerr << "No command provided.\n";
    return EXIT_FAILURE;
  }

  std::string command = argv[1];

  if (command == "cat-file") {
    std::string flag = argv[2];
    std::string blob = argv[3];

    if (flag == "-p") {
      auto header{blob.substr(0, 2)};
      auto file{blob.substr(2)};

      std::string blob_path{std::format(".git/objects/{}/{}", header, file)};

      std::ifstream blobFile(blob_path, std::ios::binary);
      if (!blobFile) {
        std::cerr << "Failed to open the file.\n";
        return EXIT_FAILURE;
      }

      std::vector<Bytef> compressedData(
          (std::istreambuf_iterator<char>(blobFile)),
          std::istreambuf_iterator<char>());

      std::vector<Bytef> decompressedData;

      try {
        decompress(compressedData, decompressedData);

        auto blob_data = getBlobData(decompressedData);

        std::cout << blob_data;
        return EXIT_SUCCESS;
      } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return EXIT_FAILURE;
      }
    };
  }

  if (command == "init") {
    try {
      std::filesystem::create_directory(".git");
      std::filesystem::create_directory(".git/objects");
      std::filesystem::create_directory(".git/refs");

      std::ofstream headFile(".git/HEAD");
      if (headFile.is_open()) {
        headFile << "ref: refs/heads/main\n";
        headFile.close();
      } else {
        std::cerr << "Failed to create .git/HEAD file.\n";
        return EXIT_FAILURE;
      }

      std::cout << "Initialized git directory\n";
    } catch (const std::filesystem::filesystem_error &e) {
      std::cerr << e.what() << '\n';
      return EXIT_FAILURE;
    }
  } else {
    std::cerr << "Unknown command " << command << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
