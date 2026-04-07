#include "perimeter/learning/qtable_checkpoint.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "perimeter/core/AgentState.h"
#include "perimeter/core/AgentType.h"
#include "perimeter/environment/Action.h"
#include "perimeter/geometry/Hex.h"
#include "perimeter/learning/nash_q_learning.h"

namespace perimeter::learning
{

using core::AgentState;
using core::AgentType;
using environment::Action;
using geometry::Hex;

class QTableCheckpointTest : public ::testing::Test
{
protected:
  void SetUp() override { testDir_ = "test_qtables_tmp"; }

  void TearDown() override
  {
    if (std::filesystem::exists(testDir_)) {
      std::filesystem::remove_all(testDir_);
    }
  }

  std::string testDir_;
};

TEST_F(QTableCheckpointTest, SaveAndLoadBasic)
{
  int agentId = 0;
  AgentType agentType = AgentType::ATTACKER;
  int numAgents = 3;
  double gamma = 0.95;

  JointActionSpace actionSpace(numAgents);
  NashQLearning learner(agentId, numAgents, gamma, actionSpace, agentType);

  JointState state1 = {AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
                       AgentState{1, AgentType::ATTACKER, Hex{1, 1}},
                       AgentState{2, AgentType::DEFENDER, Hex{-1, -1}}};

  JointAction action1 = {Action::EAST, Action::WEST, Action::STAY};
  JointAction action2 = {Action::NORTHEAST, Action::SOUTHWEST, Action::NORTHWEST};

  auto& qTable = const_cast<QTable&>(learner.getQTable());
  qTable[state1][action1] = 10.5;
  qTable[state1][action2] = 20.3;

  CheckpointMetadata metadata;
  metadata.agentId = agentId;
  metadata.agentType = agentType;
  metadata.numAgents = numAgents;
  metadata.gamma = gamma;
  metadata.radius = 5;
  metadata.attackerCount = 2;
  metadata.defenderCount = 1;
  metadata.timestamp = 123456789;
  metadata.version = 1;

  std::filesystem::create_directories(testDir_);
  std::string filepath = testDir_ + "/test_checkpoint.bin";
  QTableCheckpoint::save(learner, metadata, filepath);

  ASSERT_TRUE(std::filesystem::exists(filepath));

  NashQLearning loadedLearner(agentId, numAgents, gamma, actionSpace, agentType);
  QTableCheckpoint::load(loadedLearner, filepath, true);

  const auto& loadedQTable = loadedLearner.getQTable();
  EXPECT_EQ(loadedQTable.size(), 1);
  EXPECT_TRUE(loadedQTable.count(state1) > 0);
  EXPECT_DOUBLE_EQ(loadedQTable.at(state1).at(action1), 10.5);
  EXPECT_DOUBLE_EQ(loadedQTable.at(state1).at(action2), 20.3);
}

TEST_F(QTableCheckpointTest, MetadataValidation)
{
  int agentId = 1;
  AgentType agentType = AgentType::DEFENDER;
  int numAgents = 3;
  double gamma = 0.95;

  JointActionSpace actionSpace(numAgents);
  NashQLearning learner(agentId, numAgents, gamma, actionSpace, agentType);

  CheckpointMetadata metadata;
  metadata.agentId = agentId;
  metadata.agentType = agentType;
  metadata.numAgents = numAgents;
  metadata.gamma = gamma;
  metadata.radius = 5;
  metadata.attackerCount = 2;
  metadata.defenderCount = 1;
  metadata.timestamp = 123456789;
  metadata.version = 1;

  std::filesystem::create_directories(testDir_);
  std::string filepath = testDir_ + "/test_validation.bin";
  QTableCheckpoint::save(learner, metadata, filepath);

  NashQLearning wrongIdLearner(999, numAgents, gamma, actionSpace, agentType);
  EXPECT_THROW(QTableCheckpoint::load(wrongIdLearner, filepath, true), std::runtime_error);

  NashQLearning wrongTypeLearner(agentId, numAgents, gamma, actionSpace, AgentType::ATTACKER);
  EXPECT_THROW(QTableCheckpoint::load(wrongTypeLearner, filepath, true), std::runtime_error);

  NashQLearning wrongNumLearner(agentId, 5, gamma, actionSpace, agentType);
  EXPECT_THROW(QTableCheckpoint::load(wrongNumLearner, filepath, true), std::runtime_error);
}

TEST_F(QTableCheckpointTest, LoadWithoutValidation)
{
  int agentId = 0;
  AgentType agentType = AgentType::ATTACKER;
  int numAgents = 3;
  double gamma = 0.95;

  JointActionSpace actionSpace(numAgents);
  NashQLearning learner(agentId, numAgents, gamma, actionSpace, agentType);

  CheckpointMetadata metadata;
  metadata.agentId = agentId;
  metadata.agentType = agentType;
  metadata.numAgents = numAgents;
  metadata.gamma = gamma;
  metadata.radius = 5;
  metadata.attackerCount = 2;
  metadata.defenderCount = 1;
  metadata.timestamp = 123456789;
  metadata.version = 1;

  std::filesystem::create_directories(testDir_);
  std::string filepath = testDir_ + "/test_no_validation.bin";
  QTableCheckpoint::save(learner, metadata, filepath);

  NashQLearning wrongIdLearner(999, numAgents, gamma, actionSpace, agentType);
  EXPECT_NO_THROW(QTableCheckpoint::load(wrongIdLearner, filepath, false));
}

TEST_F(QTableCheckpointTest, ReadMetadata)
{
  int agentId = 2;
  AgentType agentType = AgentType::DEFENDER;
  int numAgents = 3;
  double gamma = 0.9;

  JointActionSpace actionSpace(numAgents);
  NashQLearning learner(agentId, numAgents, gamma, actionSpace, agentType);

  CheckpointMetadata metadata;
  metadata.agentId = agentId;
  metadata.agentType = agentType;
  metadata.numAgents = numAgents;
  metadata.gamma = gamma;
  metadata.radius = 7;
  metadata.attackerCount = 2;
  metadata.defenderCount = 1;
  metadata.timestamp = 987654321;
  metadata.version = 1;

  std::filesystem::create_directories(testDir_);
  std::string filepath = testDir_ + "/test_metadata.bin";
  QTableCheckpoint::save(learner, metadata, filepath);

  CheckpointMetadata loaded = QTableCheckpoint::readMetadata(filepath);

  EXPECT_EQ(loaded.agentId, agentId);
  EXPECT_EQ(loaded.agentType, agentType);
  EXPECT_EQ(loaded.numAgents, numAgents);
  EXPECT_DOUBLE_EQ(loaded.gamma, gamma);
  EXPECT_EQ(loaded.radius, 7);
  EXPECT_EQ(loaded.attackerCount, 2);
  EXPECT_EQ(loaded.defenderCount, 1);
  EXPECT_EQ(loaded.timestamp, 987654321ULL);
  EXPECT_EQ(loaded.version, 1U);
}

TEST_F(QTableCheckpointTest, EmptyQTable)
{
  int agentId = 0;
  AgentType agentType = AgentType::ATTACKER;
  int numAgents = 3;
  double gamma = 0.95;

  JointActionSpace actionSpace(numAgents);
  NashQLearning learner(agentId, numAgents, gamma, actionSpace, agentType);

  CheckpointMetadata metadata;
  metadata.agentId = agentId;
  metadata.agentType = agentType;
  metadata.numAgents = numAgents;
  metadata.gamma = gamma;
  metadata.radius = 5;
  metadata.attackerCount = 2;
  metadata.defenderCount = 1;
  metadata.timestamp = 123456789;
  metadata.version = 1;

  std::filesystem::create_directories(testDir_);
  std::string filepath = testDir_ + "/test_empty.bin";

  EXPECT_NO_THROW(QTableCheckpoint::save(learner, metadata, filepath));

  NashQLearning loadedLearner(agentId, numAgents, gamma, actionSpace, agentType);
  EXPECT_NO_THROW(QTableCheckpoint::load(loadedLearner, filepath, true));

  EXPECT_EQ(loadedLearner.getQTable().size(), 0);
}

TEST_F(QTableCheckpointTest, ExtractStepNumberFromAgentFile)
{
  const std::string filepath = "qtables/scenario_2a_1d_agent0_step10000.bin";
  EXPECT_EQ(QTableCheckpoint::extractStepNumber(filepath), 10000);
}

TEST_F(QTableCheckpointTest, ExtractStepNumberFromWildcardPattern)
{
  const std::string filepath = "qtables/scenario_2a_1d_agent*_step11000.bin";
  EXPECT_EQ(QTableCheckpoint::extractStepNumber(filepath), 11000);
}

TEST_F(QTableCheckpointTest, ExtractStepNumberRejectsInvalidFilename)
{
  const std::string filepath = "qtables/scenario_2a_1d_agent0_latest.bin";
  EXPECT_THROW(QTableCheckpoint::extractStepNumber(filepath), std::runtime_error);
}

TEST_F(QTableCheckpointTest, ParallelSaveAll)
{
  // Create multiple agents with some Q-table data
  int numAgents = 3;
  double gamma = 0.95;
  JointActionSpace actionSpace(numAgents);

  std::vector<NashQLearning> learners;
  for (int i = 0; i < numAgents; ++i) {
    AgentType type = (i == 0) ? AgentType::ATTACKER : AgentType::DEFENDER;
    learners.emplace_back(i, numAgents, gamma, actionSpace, type);

    // Add some Q-table entries
    JointState state = {AgentState{0, AgentType::ATTACKER, Hex{0, 0}},
                        AgentState{1, AgentType::DEFENDER, Hex{1, 1}},
                        AgentState{2, AgentType::DEFENDER, Hex{-1, -1}}};
    JointAction action = {Action::EAST, Action::STAY, Action::WEST};
    
    auto& qTable = const_cast<QTable&>(learners[i].getQTable());
    qTable[state][action] = 10.5;
  }

  // Save all agents in parallel
  environment::InitializationConfig config{5, 1, 2, 12345};
  int stepNumber = 1000;

  EXPECT_NO_THROW(QTableCheckpoint::saveAll(learners, config, gamma, stepNumber, testDir_));

  // Verify all files were created
  for (int i = 0; i < numAgents; ++i) {
    std::string expectedFile = testDir_ + "/scenario_1a_2d_agent" + std::to_string(i) + "_step1000.bin";
    EXPECT_TRUE(std::filesystem::exists(expectedFile)) << "File not found: " << expectedFile;
  }

  // Load and verify data
  std::vector<NashQLearning> loadedLearners;
  for (int i = 0; i < numAgents; ++i) {
    AgentType type = (i == 0) ? AgentType::ATTACKER : AgentType::DEFENDER;
    loadedLearners.emplace_back(i, numAgents, gamma, actionSpace, type);
  }

  std::string pattern = testDir_ + "/scenario_1a_2d_agent*_step1000.bin";
  EXPECT_NO_THROW(QTableCheckpoint::loadAll(loadedLearners, pattern));

  // Verify loaded Q-tables
  for (int i = 0; i < numAgents; ++i) {
    EXPECT_EQ(loadedLearners[i].getQTable().size(), 1);
  }
}

}  // namespace perimeter::learning
