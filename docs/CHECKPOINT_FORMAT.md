# Q-Table Checkpoint Binary Format Specification

This document describes the binary format used by the Q-Table checkpoint system for saving and loading Nash Q-Learning agent state.

## Overview

Checkpoint files use a custom binary format with the following features:
- **Magic number** for file type identification
- **Versioning** for format evolution
- **Metadata** for scenario and agent validation
- **zlib compression** for ~80-90% size reduction
- **CRC32 checksum** for integrity verification
- **Atomic writes** via temporary files

## File Structure

```
┌─────────────────────────────────────┐
│     Compressed Payload              │
│  ┌──────────────────────────────┐   │
│  │  Header (metadata)           │   │
│  │  Q_s_a_ table                │   │
│  │  N_s_ table                  │   │
│  │  N_s_a_ table                │   │
│  └──────────────────────────────┘   │
├─────────────────────────────────────┤
│     CRC32 Checksum (4 bytes)        │
└─────────────────────────────────────┘
```

The entire payload (header + tables) is compressed using zlib deflate, then the 4-byte CRC32 checksum of the compressed data is appended.

## Header Format

The header is the first section of the uncompressed payload:

| Field          | Type       | Size (bytes) | Description                              |
|----------------|------------|--------------|------------------------------------------|
| magic          | char[5]    | 5            | Magic number: "PERIQ" (identifies file)  |
| version        | uint32_t   | 4            | Format version (current: 1)              |
| timestamp      | uint64_t   | 8            | Unix timestamp (seconds since epoch)     |
| agentId        | int32_t    | 4            | Agent identifier                         |
| agentType      | uint8_t    | 1            | 0=ATTACKER, 1=DEFENDER                   |
| numAgents      | int32_t    | 4            | Total agents in scenario                 |
| gamma          | double     | 8            | Discount factor                          |
| radius         | int32_t    | 4            | Hex grid radius                          |
| attackerCount  | int32_t    | 4            | Number of attackers                      |
| defenderCount  | int32_t    | 4            | Number of defenders                      |

**Total header size**: 46 bytes (before compression)

## Data Serialization

### JointState

A JointState is a vector of AgentState structures:

```
┌────────────────┐
│ size (uint32)  │  Number of agents in state
├────────────────┤
│ AgentState[0]  │  First agent
│ AgentState[1]  │  Second agent
│ ...            │
│ AgentState[N-1]│  Last agent
└────────────────┘
```

Each AgentState contains:

| Field    | Type     | Size | Description                |
|----------|----------|------|----------------------------|
| id       | int32_t  | 4    | Agent ID                   |
| type     | uint8_t  | 1    | Agent type (0 or 1)        |
| q        | int32_t  | 4    | Hex coordinate q           |
| r        | int32_t  | 4    | Hex coordinate r           |

**AgentState size**: 13 bytes

### JointAction

A JointAction is a vector of Action enum values:

```
┌────────────────┐
│ size (uint32)  │  Number of actions
├────────────────┤
│ action[0]      │  First action (uint8)
│ action[1]      │  Second action (uint8)
│ ...            │
│ action[N-1]    │  Last action (uint8)
└────────────────┘
```

Action enum values:
- 0 = EAST
- 1 = NORTHEAST
- 2 = NORTHWEST
- 3 = WEST
- 4 = SOUTHWEST
- 5 = SOUTHEAST
- 6 = STAY

### Q-Table (Q_s_a_)

The Q-table maps (JointState, JointAction) pairs to Q-values (double):

```
┌──────────────────────┐
│ outer_size (uint32)  │  Number of states
├──────────────────────┤
│ State 0              │
│   JointState         │
│   inner_size (uint32)│  Number of actions for this state
│   ┌──────────────────┤
│   │ Action 0         │
│   │   JointAction    │
│   │   Q-value (dbl)  │
│   ├──────────────────┤
│   │ Action 1         │
│   │   ...            │
│   └──────────────────┤
├──────────────────────┤
│ State 1              │
│   ...                │
└──────────────────────┘
```

This is a nested map serialization.

### N_s_ Table

Maps JointState to visit count (uint32_t).

### N_s_a_ Table

Maps (JointState, JointAction) to visit count (uint32_t).

## Compression

After serializing header + Q_s_a_ + N_s_ + N_s_a_, the entire byte stream is compressed using zlib deflate (compression level 6).

## Checksum

A CRC32 checksum is computed over the compressed data and appended as the last 4 bytes of the file.

## Atomic Writes

To prevent corruption during save:
1. Write to `<filepath>.tmp`
2. Flush and close temporary file
3. Rename `<filepath>.tmp` to `<filepath>` (atomic on POSIX)

## File Naming Convention

Recommended naming pattern:
```
qtables/scenario_<attackers>a_<defenders>d_agent<id>_step<step>.bin
```

Examples:
- `qtables/scenario_2a_1d_agent0_step1000.bin`
- `qtables/scenario_2a_1d_agent1_step1000.bin`
