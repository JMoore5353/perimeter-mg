#include "perimeter/learning/qtable_checkpoint.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <zlib.h>

namespace perimeter::learning
{

// Helper class for writing to byte buffer
class ByteWriter
{
public:
  template<typename T>
  void write(const T& value)
  {
    const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(&value);
    buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
  }

  void writeBytes(const void* data, std::size_t size)
  {
    const std::uint8_t* ptr = reinterpret_cast<const std::uint8_t*>(data);
    buffer.insert(buffer.end(), ptr, ptr + size);
  }

  const std::vector<std::uint8_t>& getBuffer() const { return buffer; }

private:
  std::vector<std::uint8_t> buffer;
};

// Helper class for reading from byte buffer
class ByteReader
{
public:
  ByteReader(const std::vector<std::uint8_t>& data) : buffer(data), pos(0) {}

  template<typename T>
  T read()
  {
    if (pos + sizeof(T) > buffer.size()) {
      throw std::runtime_error("Buffer underflow");
    }
    T value;
    std::memcpy(&value, buffer.data() + pos, sizeof(T));
    pos += sizeof(T);
    return value;
  }

  void readBytes(void* dest, std::size_t size)
  {
    if (pos + size > buffer.size()) {
      throw std::runtime_error("Buffer underflow");
    }
    std::memcpy(dest, buffer.data() + pos, size);
    pos += size;
  }

private:
  const std::vector<std::uint8_t>& buffer;
  std::size_t pos;
};

// Header serialization
void QTableCheckpoint::writeHeader(std::ofstream& out, const CheckpointMetadata& metadata)
{
  ByteWriter writer;
  writer.writeBytes(MAGIC, 5);
  writer.write(VERSION);
  writer.write(metadata.timestamp);
  writer.write(static_cast<std::int32_t>(metadata.agentId));
  writer.write(static_cast<std::uint8_t>(metadata.agentType));
  writer.write(static_cast<std::int32_t>(metadata.numAgents));
  writer.write(metadata.gamma);
  writer.write(static_cast<std::int32_t>(metadata.radius));
  writer.write(static_cast<std::int32_t>(metadata.attackerCount));
  writer.write(static_cast<std::int32_t>(metadata.defenderCount));

  const auto& buf = writer.getBuffer();
  out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}

CheckpointMetadata QTableCheckpoint::readHeader(std::ifstream& in)
{
  // Read header size bytes
  std::size_t headerSize = 5 + 4 + 8 + 4 + 1 + 4 + 8 + 4 + 4 + 4;
  std::vector<std::uint8_t> headerData(headerSize);
  in.read(reinterpret_cast<char*>(headerData.data()), headerSize);

  ByteReader reader(headerData);

  // Verify magic number
  char magic[6] = {0};
  reader.readBytes(magic, 5);
  if (std::strncmp(magic, MAGIC, 5) != 0) {
    throw std::runtime_error("Invalid checkpoint file: magic number mismatch");
  }

  CheckpointMetadata metadata;
  metadata.version = reader.read<std::uint32_t>();
  if (metadata.version != VERSION) {
    throw std::runtime_error("Unsupported checkpoint version: " + std::to_string(metadata.version));
  }

  metadata.timestamp = reader.read<std::uint64_t>();
  metadata.agentId = reader.read<std::int32_t>();
  metadata.agentType = static_cast<core::AgentType>(reader.read<std::uint8_t>());
  metadata.numAgents = reader.read<std::int32_t>();
  metadata.gamma = reader.read<double>();
  metadata.radius = reader.read<std::int32_t>();
  metadata.attackerCount = reader.read<std::int32_t>();
  metadata.defenderCount = reader.read<std::int32_t>();

  return metadata;
}

// JointState serialization
void QTableCheckpoint::writeJointState(std::ofstream& out, const JointState& state)
{
  ByteWriter writer;
  writer.write(static_cast<std::size_t>(state.size()));
  for (const auto& agentState : state) {
    writer.write(static_cast<std::int32_t>(agentState.id));
    writer.write(static_cast<std::uint8_t>(agentState.type));
    writer.write(static_cast<std::int32_t>(agentState.position.q));
    writer.write(static_cast<std::int32_t>(agentState.position.r));
  }
  const auto& buf = writer.getBuffer();
  out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}

JointState QTableCheckpoint::readJointState(std::ifstream& in)
{
  // Read size first
  std::size_t size;
  in.read(reinterpret_cast<char*>(&size), sizeof(std::size_t));

  // Read rest of state
  std::size_t agentDataSize = size * (4 + 1 + 4 + 4);
  std::vector<std::uint8_t> data(agentDataSize);
  in.read(reinterpret_cast<char*>(data.data()), agentDataSize);

  ByteReader reader(data);
  JointState state;
  state.reserve(size);

  for (std::size_t i = 0; i < size; ++i) {
    core::AgentState agent;
    agent.id = reader.read<std::int32_t>();
    agent.type = static_cast<core::AgentType>(reader.read<std::uint8_t>());
    agent.position.q = reader.read<std::int32_t>();
    agent.position.r = reader.read<std::int32_t>();
    state.push_back(agent);
  }

  return state;
}

// JointAction serialization
void QTableCheckpoint::writeJointAction(std::ofstream& out, const JointAction& action)
{
  ByteWriter writer;
  writer.write(static_cast<std::size_t>(action.size()));
  for (const auto& a : action) {
    writer.write(static_cast<std::uint8_t>(a));
  }
  const auto& buf = writer.getBuffer();
  out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
}

JointAction QTableCheckpoint::readJointAction(std::ifstream& in)
{
  std::size_t size;
  in.read(reinterpret_cast<char*>(&size), sizeof(std::size_t));

  std::vector<std::uint8_t> data(size);
  in.read(reinterpret_cast<char*>(data.data()), size);

  ByteReader reader(data);
  JointAction action;
  action.reserve(size);

  for (std::size_t i = 0; i < size; ++i) {
    action.push_back(static_cast<environment::Action>(reader.read<std::uint8_t>()));
  }

  return action;
}

// Q-table serialization
void QTableCheckpoint::writeQTable(std::ofstream& out, const QTable& table)
{
  ByteWriter writer;
  writer.write(static_cast<std::size_t>(table.size()));
  const auto& buf = writer.getBuffer();
  out.write(reinterpret_cast<const char*>(buf.data()), buf.size());

  for (const auto& [state, innerMap] : table) {
    writeJointState(out, state);

    ByteWriter innerWriter;
    innerWriter.write(static_cast<std::size_t>(innerMap.size()));
    const auto& innerBuf = innerWriter.getBuffer();
    out.write(reinterpret_cast<const char*>(innerBuf.data()), innerBuf.size());

    for (const auto& [action, qValue] : innerMap) {
      writeJointAction(out, action);
      out.write(reinterpret_cast<const char*>(&qValue), sizeof(double));
    }
  }
}

void QTableCheckpoint::readQTable(std::ifstream& in, QTable& table)
{
  table.clear();
  std::size_t outerSize;
  in.read(reinterpret_cast<char*>(&outerSize), sizeof(std::size_t));

  for (std::size_t i = 0; i < outerSize; ++i) {
    JointState state = readJointState(in);

    std::size_t innerSize;
    in.read(reinterpret_cast<char*>(&innerSize), sizeof(std::size_t));

    std::unordered_map<JointAction, double, ActionVectorHash> innerMap;
    for (std::size_t j = 0; j < innerSize; ++j) {
      JointAction action = readJointAction(in);
      double qValue;
      in.read(reinterpret_cast<char*>(&qValue), sizeof(double));
      innerMap[action] = qValue;
    }

    table[state] = std::move(innerMap);
  }
}

// N_s_ table serialization
void QTableCheckpoint::writeNsTable(std::ofstream& out, const NsTable& table)
{
  ByteWriter writer;
  writer.write(static_cast<std::size_t>(table.size()));
  const auto& buf = writer.getBuffer();
  out.write(reinterpret_cast<const char*>(buf.data()), buf.size());

  for (const auto& [state, count] : table) {
    writeJointState(out, state);
    std::int32_t count32 = static_cast<std::int32_t>(count);
    out.write(reinterpret_cast<const char*>(&count32), sizeof(std::int32_t));
  }
}

void QTableCheckpoint::readNsTable(std::ifstream& in, NsTable& table)
{
  table.clear();
  std::size_t size;
  in.read(reinterpret_cast<char*>(&size), sizeof(std::size_t));

  for (std::size_t i = 0; i < size; ++i) {
    JointState state = readJointState(in);
    std::int32_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(std::int32_t));
    table[state] = count;
  }
}

// N_s_a_ table serialization
void QTableCheckpoint::writeNsaTable(std::ofstream& out, const NsaTable& table)
{
  ByteWriter writer;
  writer.write(static_cast<std::size_t>(table.size()));
  const auto& buf = writer.getBuffer();
  out.write(reinterpret_cast<const char*>(buf.data()), buf.size());

  for (const auto& [state, innerMap] : table) {
    writeJointState(out, state);

    ByteWriter innerWriter;
    innerWriter.write(static_cast<std::size_t>(innerMap.size()));
    const auto& innerBuf = innerWriter.getBuffer();
    out.write(reinterpret_cast<const char*>(innerBuf.data()), innerBuf.size());

    for (const auto& [action, count] : innerMap) {
      writeJointAction(out, action);
      std::int32_t count32 = static_cast<std::int32_t>(count);
      out.write(reinterpret_cast<const char*>(&count32), sizeof(std::int32_t));
    }
  }
}

void QTableCheckpoint::readNsaTable(std::ifstream& in, NsaTable& table)
{
  table.clear();
  std::size_t outerSize;
  in.read(reinterpret_cast<char*>(&outerSize), sizeof(std::size_t));

  for (std::size_t i = 0; i < outerSize; ++i) {
    JointState state = readJointState(in);

    std::size_t innerSize;
    in.read(reinterpret_cast<char*>(&innerSize), sizeof(std::size_t));

    std::unordered_map<JointAction, int, ActionVectorHash> innerMap;
    for (std::size_t j = 0; j < innerSize; ++j) {
      JointAction action = readJointAction(in);
      std::int32_t count;
      in.read(reinterpret_cast<char*>(&count), sizeof(std::int32_t));
      innerMap[action] = count;
    }

    table[state] = std::move(innerMap);
  }
}

// Compression
std::vector<std::uint8_t> QTableCheckpoint::compress(const std::vector<std::uint8_t>& data)
{
  uLongf compressedSize = compressBound(data.size());
  std::vector<std::uint8_t> compressed(compressedSize + sizeof(std::uint64_t));

  // Store original size at beginning
  std::uint64_t originalSize = data.size();
  std::memcpy(compressed.data(), &originalSize, sizeof(std::uint64_t));

  int result = compress2(compressed.data() + sizeof(std::uint64_t), &compressedSize, data.data(),
                         data.size(), Z_BEST_COMPRESSION);

  if (result != Z_OK) {
    throw std::runtime_error("Compression failed with error code: " + std::to_string(result));
  }

  compressed.resize(compressedSize + sizeof(std::uint64_t));
  return compressed;
}

std::vector<std::uint8_t> QTableCheckpoint::decompress(const std::vector<std::uint8_t>& data)
{
  if (data.size() < sizeof(std::uint64_t)) {
    throw std::runtime_error("Invalid compressed data: too small");
  }

  // Read original size
  std::uint64_t originalSize;
  std::memcpy(&originalSize, data.data(), sizeof(std::uint64_t));

  std::vector<std::uint8_t> decompressed(originalSize);
  uLongf decompressedSize = originalSize;

  int result = uncompress(decompressed.data(), &decompressedSize,
                          data.data() + sizeof(std::uint64_t), data.size() - sizeof(std::uint64_t));

  if (result != Z_OK) {
    throw std::runtime_error("Decompression failed with error code: " + std::to_string(result));
  }

  return decompressed;
}

// CRC32 checksum
std::uint32_t QTableCheckpoint::computeCRC32(const std::vector<std::uint8_t>& data)
{
  return crc32(0L, data.data(), data.size());
}

// High-level save function
void QTableCheckpoint::save(const NashQLearning& learner, const CheckpointMetadata& metadata,
                            const std::string& filepath)
{
  // Create directory if needed
  std::filesystem::path path(filepath);
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  // Write to temporary file first (for atomicity)
  std::string tempPath = filepath + ".tmp";
  std::ofstream out(tempPath, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Failed to open file for writing: " + tempPath);
  }

  // Write header and Q-tables
  writeHeader(out, metadata);
  writeQTable(out, learner.getQTable());
  writeNsTable(out, learner.getNsTable());
  writeNsaTable(out, learner.getNsaTable());

  out.close();

  // Read back for compression and checksum
  std::ifstream inTemp(tempPath, std::ios::binary);
  std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(inTemp)),
                                  std::istreambuf_iterator<char>());
  inTemp.close();

  // Compress
  std::vector<std::uint8_t> compressed = compress(data);

  // Compute checksum
  std::uint32_t checksum = computeCRC32(compressed);

  // Write final file
  std::ofstream finalOut(filepath, std::ios::binary);
  if (!finalOut) {
    throw std::runtime_error("Failed to open file for writing: " + filepath);
  }

  finalOut.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
  finalOut.write(reinterpret_cast<const char*>(&checksum), sizeof(std::uint32_t));
  finalOut.close();

  // Remove temp file
  std::filesystem::remove(tempPath);
}

// High-level load function
void QTableCheckpoint::load(NashQLearning& learner, const std::string& filepath,
                            bool validateMetadata)
{
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open checkpoint file: " + filepath);
  }

  // Read entire file
  in.seekg(0, std::ios::end);
  std::size_t fileSize = in.tellg();
  in.seekg(0, std::ios::beg);

  if (fileSize < sizeof(std::uint32_t)) {
    throw std::runtime_error("Checkpoint file too small");
  }

  std::vector<std::uint8_t> fileData(fileSize);
  in.read(reinterpret_cast<char*>(fileData.data()), fileSize);
  in.close();

  // Extract checksum
  std::uint32_t storedChecksum;
  std::memcpy(&storedChecksum, fileData.data() + fileSize - sizeof(std::uint32_t),
              sizeof(std::uint32_t));

  std::vector<std::uint8_t> compressed(fileData.begin(), fileData.end() - sizeof(std::uint32_t));

  // Verify checksum
  std::uint32_t computedChecksum = computeCRC32(compressed);
  if (computedChecksum != storedChecksum) {
    throw std::runtime_error("Checkpoint file corrupted: checksum mismatch");
  }

  // Decompress
  std::vector<std::uint8_t> data = decompress(compressed);

  // Write to temporary file for reading
  std::string tempPath = filepath + ".load.tmp";
  std::ofstream tempOut(tempPath, std::ios::binary);
  tempOut.write(reinterpret_cast<const char*>(data.data()), data.size());
  tempOut.close();

  std::ifstream tempIn(tempPath, std::ios::binary);

  // Read header
  CheckpointMetadata metadata = readHeader(tempIn);

  // Validate metadata if requested
  if (validateMetadata) {
    if (metadata.agentId != learner.getId()) {
      throw std::runtime_error("Agent ID mismatch: expected " + std::to_string(learner.getId()) +
                               ", got " + std::to_string(metadata.agentId));
    }
    if (metadata.agentType != learner.getAgentType()) {
      throw std::runtime_error("Agent type mismatch");
    }
    if (metadata.numAgents != learner.getNumAgents()) {
      throw std::runtime_error("Number of agents mismatch");
    }
    if (std::abs(metadata.gamma - learner.getGamma()) > 1e-9) {
      std::cerr << "Warning: Gamma mismatch (checkpoint: " << metadata.gamma << ", learner: "
                << learner.getGamma() << ")" << std::endl;
    }
  }

  // Read Q-tables
  QTable qTable;
  NsTable nsTable;
  NsaTable nsaTable;

  readQTable(tempIn, qTable);
  readNsTable(tempIn, nsTable);
  readNsaTable(tempIn, nsaTable);

  tempIn.close();
  std::filesystem::remove(tempPath);

  // Load into learner
  learner.setQTable(std::move(qTable));
  learner.setNsTable(std::move(nsTable));
  learner.setNsaTable(std::move(nsaTable));
}

// Save all agents
void QTableCheckpoint::saveAll(const std::vector<NashQLearning>& learners,
                               const environment::InitializationConfig& config, double gamma,
                               int stepNumber, const std::string& baseDir)
{
  std::filesystem::create_directories(baseDir);

  std::uint64_t timestamp =
    std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  for (const auto& learner : learners) {
    CheckpointMetadata metadata;
    metadata.agentId = learner.getId();
    metadata.agentType = learner.getAgentType();
    metadata.numAgents = learner.getNumAgents();
    metadata.gamma = gamma;
    metadata.radius = config.radius;
    metadata.attackerCount = config.attackerCount;
    metadata.defenderCount = config.defenderCount;
    metadata.timestamp = timestamp;
    metadata.version = VERSION;

    std::ostringstream filename;
    filename << baseDir << "/scenario_" << config.attackerCount << "a_" << config.defenderCount
             << "d_agent" << learner.getId() << "_step" << stepNumber << ".bin";

    save(learner, metadata, filename.str());
  }
}

// Load all agents
std::vector<CheckpointMetadata> QTableCheckpoint::loadAll(std::vector<NashQLearning>& learners,
                                                           const std::string& filepath)
{
  std::vector<CheckpointMetadata> metadataList;

  // Extract pattern from filepath
  std::filesystem::path path(filepath);
  std::string pattern = path.filename().string();

  // Replace wildcard with regex
  pattern = std::regex_replace(pattern, std::regex("agent\\*"), "agent(\\d+)");
  std::regex fileRegex(pattern);

  // Find all matching files
  std::vector<std::string> matchingFiles;
  for (const auto& entry : std::filesystem::directory_iterator(path.parent_path())) {
    std::smatch match;
    std::string filename = entry.path().filename().string();
    if (std::regex_match(filename, match, fileRegex)) {
      matchingFiles.push_back(entry.path().string());
    }
  }

  std::sort(matchingFiles.begin(), matchingFiles.end());

  if (matchingFiles.size() != learners.size()) {
    throw std::runtime_error("Number of checkpoint files (" + std::to_string(matchingFiles.size()) +
                             ") does not match number of learners (" +
                             std::to_string(learners.size()) + ")");
  }

  for (std::size_t i = 0; i < learners.size(); ++i) {
    load(learners[i], matchingFiles[i], true);
    metadataList.push_back(readMetadata(matchingFiles[i]));
  }

  return metadataList;
}

// Find latest checkpoint
std::string QTableCheckpoint::findLatestCheckpoint(const std::string& baseDir, int attackers,
                                                    int defenders)
{
  std::ostringstream pattern;
  pattern << "scenario_" << attackers << "a_" << defenders << "d_agent0_step(\\d+)\\.bin";
  std::regex fileRegex(pattern.str());

  int maxStep = -1;
  std::string latestFile;

  if (!std::filesystem::exists(baseDir)) {
    return "";
  }

  for (const auto& entry : std::filesystem::directory_iterator(baseDir)) {
    std::smatch match;
    std::string filename = entry.path().filename().string();
    if (std::regex_match(filename, match, fileRegex)) {
      int step = std::stoi(match[1]);
      if (step > maxStep) {
        maxStep = step;
        latestFile = entry.path().parent_path().string() + "/scenario_" + std::to_string(attackers) +
                     "a_" + std::to_string(defenders) + "d_agent*_step" + std::to_string(step) +
                     ".bin";
      }
    }
  }

  return latestFile;
}

int QTableCheckpoint::extractStepNumber(const std::string& filepath)
{
  std::filesystem::path path(filepath);
  const std::string filename = path.filename().string();
  std::smatch match;
  const std::regex stepRegex(".*_step(\\d+)\\.bin$");
  if (!std::regex_match(filename, match, stepRegex)) {
    throw std::runtime_error("Failed to parse checkpoint step from filename: " + filename);
  }
  return std::stoi(match[1]);
}

// Read metadata only
CheckpointMetadata QTableCheckpoint::readMetadata(const std::string& filepath)
{
  std::ifstream in(filepath, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open checkpoint file: " + filepath);
  }

  // Read entire file
  in.seekg(0, std::ios::end);
  std::size_t fileSize = in.tellg();
  in.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> fileData(fileSize);
  in.read(reinterpret_cast<char*>(fileData.data()), fileSize);
  in.close();

  // Extract compressed data (without checksum)
  std::vector<std::uint8_t> compressed(fileData.begin(), fileData.end() - sizeof(std::uint32_t));

  // Decompress
  std::vector<std::uint8_t> data = decompress(compressed);

  // Write to temp file
  std::string tempPath = filepath + ".meta.tmp";
  std::ofstream tempOut(tempPath, std::ios::binary);
  tempOut.write(reinterpret_cast<const char*>(data.data()), data.size());
  tempOut.close();

  std::ifstream tempIn(tempPath, std::ios::binary);
  CheckpointMetadata metadata = readHeader(tempIn);
  tempIn.close();

  std::filesystem::remove(tempPath);

  return metadata;
}

} // namespace perimeter::learning
