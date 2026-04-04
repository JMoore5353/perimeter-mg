#ifndef PERIMETER_PERIMETER_LEARNING_QTABLE_CHECKPOINT_H
#define PERIMETER_PERIMETER_LEARNING_QTABLE_CHECKPOINT_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "perimeter/core/AgentState.h"
#include "perimeter/environment/Action.h"
#include "perimeter/environment/Initialization.h"
#include "perimeter/learning/joint.h"
#include "perimeter/learning/nash_q_learning.h"

namespace perimeter::learning
{

/**
 * Metadata stored in checkpoint files.
 * Contains scenario configuration and agent information for validation during loading.
 */
struct CheckpointMetadata
{
  int agentId;               // Agent identifier (must match learner ID when loading)
  core::AgentType agentType; // ATTACKER or DEFENDER (enforces type safety)
  int numAgents;             // Total agents in scenario (must match when loading)
  double gamma;              // Discount factor (must match when loading)
  int radius;                // Hex grid radius
  int attackerCount;         // Number of attackers in scenario
  int defenderCount;         // Number of defenders in scenario
  std::uint64_t timestamp;   // Unix timestamp when checkpoint was created
  std::uint32_t version;     // File format version (for future compatibility)
};

/**
 * Q-Table checkpoint system for Nash Q-Learning agents.
 *
 * Provides save/load functionality for Q-tables with:
 * - Binary serialization with zlib compression (~80-90% size reduction)
 * - CRC32 integrity checking to detect corruption
 * - Metadata validation to prevent loading incompatible checkpoints
 * - Atomic file writes via temporary files
 * - Support for multi-agent scenarios with pattern-based loading
 *
 * File naming convention: qtables/scenario_<A>a_<D>d_agent<id>_step<N>.bin
 * where A = attackers, D = defenders, id = agent ID, N = step number
 *
 * Example usage:
 *   // Save during training
 *   CheckpointMetadata meta{agentId, agentType, numAgents, gamma, radius, ...};
 *   QTableCheckpoint::save(learner, meta, "qtables/scenario_2a_1d_agent0_step1000.bin");
 *
 *   // Resume training
 *   QTableCheckpoint::load(learner, "qtables/scenario_2a_1d_agent0_step1000.bin");
 */
class QTableCheckpoint
{
public:
  /**
   * Save a single agent's Q-table to a binary file.
   *
   * Serializes Q_s_a_, N_s_, and N_s_a_ tables along with metadata.
   * File is written atomically using a temporary file to prevent corruption.
   * Data is compressed with zlib and includes a CRC32 checksum.
   *
   * @param learner The NashQLearning agent to save
   * @param metadata Scenario and agent metadata
   * @param filepath Output file path (created if doesn't exist)
   * @throws std::runtime_error if file cannot be written or compression fails
   */
  static void save(const NashQLearning& learner, const CheckpointMetadata& metadata,
                   const std::string& filepath);

  /**
   * Load a Q-table from a binary file into an existing learner.
   *
   * Replaces the learner's Q_s_a_, N_s_, and N_s_a_ tables with checkpoint data.
   * By default, validates that agent ID, type, numAgents, and gamma match.
   *
   * @param learner The NashQLearning agent to load into (must be initialized)
   * @param filepath Checkpoint file to load
   * @param validateMetadata If true, throws on metadata mismatch (default: true)
   * @throws std::runtime_error if file doesn't exist, is corrupted, or validation fails
   */
  static void load(NashQLearning& learner, const std::string& filepath,
                   bool validateMetadata = true);

  /**
   * Save all agents in a training run to separate checkpoint files.
   *
   * Creates one file per agent with naming pattern:
   *   <baseDir>/scenario_<A>a_<D>d_agent<id>_step<N>.bin
   *
   * @param learners Vector of all agents to save
   * @param config Environment initialization config (for radius, attacker/defender counts)
   * @param gamma Discount factor used in training
   * @param stepNumber Current training step (included in filename)
   * @param baseDir Output directory (created if doesn't exist, default: "qtables")
   * @throws std::runtime_error if directory cannot be created or any save fails
   */
  static void saveAll(const std::vector<NashQLearning>& learners,
                      const environment::InitializationConfig& config, double gamma, int stepNumber,
                      const std::string& baseDir = "qtables");

  /**
   * Load all agents from checkpoint files matching a pattern.
   *
   * Pattern should use "agent*" wildcard to match all agents in a scenario.
   * Example: "qtables/scenario_2a_1d_agent*_step1000.bin" loads agents 0, 1, 2, ...
   *
   * Metadata validation is enabled by default. Learners vector must be pre-allocated
   * with correct agent IDs and types.
   *
   * @param learners Pre-allocated vector of learners to load into
   * @param filepath Pattern with "agent*" wildcard
   * @return Vector of metadata structs (one per loaded agent)
   * @throws std::runtime_error if no files match pattern or loading fails
   */
  static std::vector<CheckpointMetadata> loadAll(std::vector<NashQLearning>& learners,
                                                 const std::string& filepath);

  /**
   * Find the most recent checkpoint for a scenario.
   *
   * Searches baseDir for files matching "scenario_<A>a_<D>d_agent*_step*.bin"
   * and returns the path with the highest step number.
   *
   * @param baseDir Directory to search
   * @param attackers Number of attackers in scenario
   * @param defenders Number of defenders in scenario
   * @return Path to latest checkpoint, or empty string if none found
   */
  static std::string findLatestCheckpoint(const std::string& baseDir, int attackers, int defenders);

  /**
   * Extract the training step number from a checkpoint filename/pattern.
   *
   * Expected suffix format: "_step<N>.bin", e.g.:
   * - scenario_2a_1d_agent0_step10000.bin
   * - scenario_2a_1d_agent*_step10000.bin
   *
   * @param filepath Checkpoint path or filename
   * @return Parsed step number N
   * @throws std::runtime_error if suffix does not match expected format
   */
  static int extractStepNumber(const std::string& filepath);

  /**
   * Read checkpoint metadata without loading the full Q-table.
   *
   * Useful for inspecting checkpoint files or validating compatibility
   * before committing to a full load.
   *
   * @param filepath Checkpoint file to read
   * @return Metadata struct with scenario and agent information
   * @throws std::runtime_error if file doesn't exist or header is invalid
   */
  static CheckpointMetadata readMetadata(const std::string& filepath);

private:
  // Magic number for file identification
  static constexpr char MAGIC[6] = "PERIQ";
  static constexpr std::uint32_t VERSION = 1;

  // Binary serialization helpers
  static void writeHeader(std::ofstream& out, const CheckpointMetadata& metadata);
  static CheckpointMetadata readHeader(std::ifstream& in);

  static void writeJointState(std::ofstream& out, const JointState& state);
  static JointState readJointState(std::ifstream& in);

  static void writeJointAction(std::ofstream& out, const JointAction& action);
  static JointAction readJointAction(std::ifstream& in);

  static void writeQTable(std::ofstream& out, const QTable& table);
  static void readQTable(std::ifstream& in, QTable& table);

  static void writeNsTable(std::ofstream& out, const NsTable& table);
  static void readNsTable(std::ifstream& in, NsTable& table);

  static void writeNsaTable(std::ofstream& out, const NsaTable& table);
  static void readNsaTable(std::ifstream& in, NsaTable& table);

  // Compression helpers
  static std::vector<std::uint8_t> compress(const std::vector<std::uint8_t>& data);
  static std::vector<std::uint8_t> decompress(const std::vector<std::uint8_t>& data);

  // Checksum helpers
  static std::uint32_t computeCRC32(const std::vector<std::uint8_t>& data);

  // Utility to write/read primitive types
  template<typename T>
  static void write(std::ofstream& out, const T& value);

  template<typename T>
  static T read(std::ifstream& in);
};

} // namespace perimeter::learning

#endif // PERIMETER_PERIMETER_LEARNING_QTABLE_CHECKPOINT_H
