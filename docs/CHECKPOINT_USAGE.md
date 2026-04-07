# Q-Table Checkpoint Save/Resume Guide

This guide explains how to save and resume Nash Q-Learning training using the checkpoint system.

## Quick Start

### Saving Checkpoints During Training

The simulator automatically saves checkpoints at regular intervals when you specify `--save-interval`:

```bash
# Save every 1000 steps
./perimeter_mg --steps 10000 --save-interval 1000

# Custom checkpoint directory
./perimeter_mg --steps 10000 --save-interval 1000 --checkpoint-dir my_training
```

Checkpoint files are saved to `qtables/` (or your custom directory) with names like:
```
qtables/scenario_2a_1d_agent0_step1000.bin
qtables/scenario_2a_1d_agent1_step1000.bin
qtables/scenario_2a_1d_agent2_step1000.bin
```

### Resuming Training

To resume from the latest checkpoint:

```bash
# Load the latest checkpoint and continue training
./perimeter_mg --load-qtable qtables/scenario_2a_1d_agent*_step1000.bin --steps 10000 --save-interval 1000
```

The `agent*` wildcard loads all agents from that checkpoint. Training continues from step 1000.

## Command-Line Options

### `--save-interval <N>`
Save checkpoints every N steps. Default: disabled (no saving).

```bash
./perimeter_mg --save-interval 500  # Save every 500 steps
```

### `--checkpoint-dir <path>`
Directory to save/load checkpoints. Default: `qtables/`.

```bash
./perimeter_mg --checkpoint-dir runs/experiment_1 --save-interval 1000
```

### `--load-qtable <pattern>`
Load Q-tables from checkpoint files. Use `agent*` wildcard to load all agents.

```bash
# Load specific step
./perimeter_mg --load-qtable qtables/scenario_2a_1d_agent*_step5000.bin

# Load from custom directory
./perimeter_mg --load-qtable my_training/scenario_2a_1d_agent*_step2000.bin
```

### `--steps <N>`
Total training steps to run. When resuming, this is additional steps beyond the checkpoint.

```bash
# Run 5000 more steps after loading step 1000 checkpoint
./perimeter_mg --load-qtable qtables/scenario_2a_1d_agent*_step1000.bin --steps 5000
```

## Typical Workflows

### Workflow 1: Long Training with Periodic Saves

```bash
# Start fresh training, save every 1000 steps
./perimeter_mg --steps 50000 --save-interval 1000

# If interrupted, resume from latest
./perimeter_mg --load-qtable qtables/scenario_2a_1d_agent*_step*.bin --steps 50000 --save-interval 1000
```

### Workflow 2: Multi-Stage Training

```bash
# Stage 1: Initial exploration (10k steps)
./perimeter_mg --steps 10000 --save-interval 2000 --checkpoint-dir stage1

# Stage 2: Refinement (load from stage1, train 10k more)
./perimeter_mg --load-qtable stage1/scenario_2a_1d_agent*_step10000.bin \
               --steps 10000 --save-interval 2000 --checkpoint-dir stage2

# Stage 3: Fine-tuning (load from stage2, train 5k more)
./perimeter_mg --load-qtable stage2/scenario_2a_1d_agent*_step10000.bin \
               --steps 5000 --save-interval 1000 --checkpoint-dir stage3
```

### Workflow 3: Experiment Comparison

```bash
# Baseline run
./perimeter_mg --steps 20000 --save-interval 5000 --checkpoint-dir baseline

# Experimental run 1 (load baseline, change parameters)
./perimeter_mg --load-qtable baseline/scenario_2a_1d_agent*_step10000.bin \
               --steps 10000 --checkpoint-dir experiment_1

# Experimental run 2 (load same baseline, different parameters)
./perimeter_mg --load-qtable baseline/scenario_2a_1d_agent*_step10000.bin \
               --steps 10000 --checkpoint-dir experiment_2
```

## Checkpoint Inspection

Use the Python parser to inspect checkpoint contents without loading into the simulator:

```bash
# View metadata and statistics
python3 scripts/parse_qtable.py qtables/scenario_2a_1d_agent0_step1000.bin

# Export to JSON for analysis
python3 scripts/parse_qtable.py qtables/scenario_2a_1d_agent0_step1000.bin --json output.json
```

Output example:
```
=== Checkpoint Metadata ===
Agent ID: 0
Agent Type: ATTACKER
Scenario: 2 attackers, 1 defenders (radius=5)
Gamma: 0.95
Timestamp: 2026-04-03 14:05:23
Version: 1

=== Q-Table Statistics ===
Number of states: 1543
Total state-action pairs: 10801
Average actions per state: 7.0
Q-value range: [-12.4, 34.2]
Mean Q-value: 5.8
```

## Validation and Safety

### Automatic Validation

When loading, the system validates:
- ✅ Agent ID matches
- ✅ Agent type (ATTACKER/DEFENDER) matches
- ✅ Number of agents matches
- ✅ Gamma (discount factor) matches
- ✅ File integrity (CRC32 checksum)

If any check fails, loading aborts with an error message:
```
Error: Agent ID mismatch: checkpoint has 0, learner has 1
Error: Checksum validation failed (file may be corrupted)
Error: Agent type mismatch: checkpoint has ATTACKER, learner has DEFENDER
```

### Disabling Validation (Advanced)

You can disable validation in code if you need to load mismatched checkpoints:

```cpp
// Load without validation (use with caution!)
QTableCheckpoint::load(learner, filepath, false);
```

This is useful for:
- Transferring knowledge between similar but not identical scenarios
- Debugging checkpoint files
- Experimenting with cross-agent learning

### Atomic Saves

Checkpoints are written atomically:
1. Data is written to a temporary file (`.tmp` extension)
2. File is flushed and closed
3. Temporary file is renamed to final name

If the program crashes during save, the previous checkpoint remains intact.

## File Organization

Recommended directory structure:

```
project/
├── perimeter_mg          # Simulator executable
├── qtables/              # Default checkpoint directory
│   ├── scenario_2a_1d_agent0_step1000.bin
│   ├── scenario_2a_1d_agent0_step2000.bin
│   ├── scenario_2a_1d_agent1_step1000.bin
│   ├── scenario_2a_1d_agent1_step2000.bin
│   └── scenario_2a_1d_agent2_step1000.bin
├── experiments/
│   ├── run_1/           # Isolated experiment
│   │   └── ...
│   └── run_2/
│       └── ...
└── scripts/
    └── parse_qtable.py  # Checkpoint inspector
```

## Troubleshooting

### Problem: "No files match pattern"

**Cause**: Incorrect wildcard pattern or wrong directory.

**Solution**: Verify files exist and use correct pattern:
```bash
ls qtables/scenario_2a_1d_agent*_step1000.bin  # Check files exist
./perimeter_mg --load-qtable "qtables/scenario_2a_1d_agent*_step1000.bin"  # Quote pattern
```

### Problem: "Agent ID mismatch"

**Cause**: Checkpoint was saved with different agent configuration.

**Solution**: Ensure scenario matches (same number of attackers/defenders):
```bash
# Wrong: trying to load 2a_1d checkpoint into 3a_2d scenario
./perimeter_mg --load-qtable qtables/scenario_2a_1d_agent*.bin  # Won't work

# Right: load checkpoint from same scenario
./perimeter_mg --load-qtable qtables/scenario_2a_1d_agent*.bin  # Works
```

### Problem: "Checksum validation failed"

**Cause**: File corruption (disk error, incomplete transfer, etc.).

**Solution**: 
1. Delete corrupted checkpoint
2. Use previous checkpoint
3. Restart training from earlier step

```bash
# Find available checkpoints
ls qtables/*.bin

# Load earlier checkpoint
./perimeter_mg --load-qtable qtables/scenario_2a_1d_agent*_step1000.bin
```

### Problem: Checkpoint files are large

**Cause**: Q-tables grow with exploration. Compression reduces size ~90% but large tables still produce large files.

**Solution**: 
- Increase `--save-interval` to save less frequently
- Clean up old checkpoints periodically
- Use external compression (gzip) for archival:
  ```bash
  tar -czf qtables_backup.tar.gz qtables/
  ```

## Performance Tips

1. **Save interval**: Balance safety vs. performance
   - Too frequent (e.g., every 10 steps): High I/O overhead
   - Too infrequent (e.g., every 100k steps): Risk losing progress
   - Recommended: 500-5000 steps depending on training length

2. **Parallel saving**: Checkpoint saves are automatically parallelized
   - Each agent's Q-table is saved in a separate thread
   - Provides ~2.5-3x speedup for 3-agent scenarios
   - No configuration needed - works out of the box

3. **Disk space**: Monitor checkpoint directory size
   ```bash
   du -sh qtables/
   ```

4. **Loading time**: Loading checkpoints adds ~0.1-1 second startup time (depends on Q-table size)

5. **Concurrent runs**: Use separate checkpoint directories to avoid conflicts
   ```bash
   ./perimeter_mg --checkpoint-dir run_A --save-interval 1000 &
   ./perimeter_mg --checkpoint-dir run_B --save-interval 1000 &
   ```

6. **Compression**: Uses Z_DEFAULT_COMPRESSION (level 6) for optimal speed/size tradeoff
   - Provides ~85-90% size reduction
   - 40-60% faster than maximum compression
   - Files are only 5-10% larger than maximum compression

## Integration with Custom Code

If you're writing custom training loops, you can use the checkpoint API directly:

```cpp
#include "perimeter/learning/qtable_checkpoint.h"

// Save checkpoint
CheckpointMetadata metadata{
  .agentId = 0,
  .agentType = AgentType::ATTACKER,
  .numAgents = 3,
  .gamma = 0.95,
  .radius = 5,
  .attackerCount = 2,
  .defenderCount = 1,
  .timestamp = static_cast<uint64_t>(std::time(nullptr)),
  .version = 1
};
QTableCheckpoint::save(learner, metadata, "checkpoint.bin");

// Load checkpoint
QTableCheckpoint::load(learner, "checkpoint.bin", true);

// Read metadata only
auto meta = QTableCheckpoint::readMetadata("checkpoint.bin");
std::cout << "Checkpoint step: " << meta.timestamp << std::endl;
```

See `include/perimeter/learning/qtable_checkpoint.h` for full API documentation.
