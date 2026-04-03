#!/usr/bin/env python3
"""
Parse and display Perimeter-MG Q-table checkpoint files.

This script reads binary checkpoint files created by the QTableCheckpoint system
and displays metadata, statistics, and optionally exports to JSON format.

Features:
- Display checkpoint metadata (agent ID, type, scenario config, timestamp)
- Compute Q-table statistics (size, value ranges, averages)
- Export full Q-table data to JSON for external analysis
- Validate file integrity (magic number, checksum)

Usage:
    # Display metadata and statistics
    python3 scripts/parse_qtable.py qtables/scenario_2a_1d_agent0_step1000.bin

    # Export to JSON for analysis
    python3 scripts/parse_qtable.py checkpoint.bin --json output.json

    # Process multiple files
    for f in qtables/*.bin; do
        python3 scripts/parse_qtable.py "$f"
    done

Output:
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

Dependencies:
    - Python 3.6+
    - Standard library only (struct, zlib, json)

Binary format details: See docs/CHECKPOINT_FORMAT.md
"""

import struct
import sys
import zlib
import json
from pathlib import Path


class QTableParser:
    """Parser for Q-table checkpoint binary format."""
    
    MAGIC = b"PERIQ"
    VERSION = 1
    
    # Type codes
    ATTACKER = 0
    DEFENDER = 1
    
    def __init__(self, filepath):
        self.filepath = Path(filepath)
        self.metadata = {}
        self.q_table = {}
        self.ns_table = {}
        self.nsa_table = {}
        
    def parse(self):
        """Load and parse the checkpoint file."""
        with open(self.filepath, 'rb') as f:
            data = f.read()
        
        # Extract checksum
        checksum_stored = struct.unpack('<I', data[-4:])[0]
        compressed = data[:-4]
        
        # Verify checksum
        checksum_computed = zlib.crc32(compressed)
        if checksum_computed != checksum_stored:
            raise ValueError(f"Checksum mismatch: {checksum_computed:08x} != {checksum_stored:08x}")
        
        # Decompress
        original_size = struct.unpack('<Q', compressed[:8])[0]
        decompressed = zlib.decompress(compressed[8:])
        
        if len(decompressed) != original_size:
            raise ValueError(f"Decompressed size mismatch: {len(decompressed)} != {original_size}")
        
        # Parse decompressed data
        self._parse_data(decompressed)
        
    def _parse_data(self, data):
        """Parse the decompressed binary data."""
        pos = 0
        
        # Read header
        magic = data[pos:pos+5]
        if magic != self.MAGIC:
            raise ValueError(f"Invalid magic number: {magic}")
        pos += 5
        
        version, = struct.unpack('<I', data[pos:pos+4])
        pos += 4
        if version != self.VERSION:
            raise ValueError(f"Unsupported version: {version}")
        
        timestamp, = struct.unpack('<Q', data[pos:pos+8])
        pos += 8
        
        agent_id, = struct.unpack('<i', data[pos:pos+4])
        pos += 4
        
        agent_type, = struct.unpack('<B', data[pos:pos+1])
        pos += 1
        
        num_agents, = struct.unpack('<i', data[pos:pos+4])
        pos += 4
        
        gamma, = struct.unpack('<d', data[pos:pos+8])
        pos += 8
        
        radius, = struct.unpack('<i', data[pos:pos+4])
        pos += 4
        
        attacker_count, = struct.unpack('<i', data[pos:pos+4])
        pos += 4
        
        defender_count, = struct.unpack('<i', data[pos:pos+4])
        pos += 4
        
        self.metadata = {
            'version': version,
            'timestamp': timestamp,
            'agent_id': agent_id,
            'agent_type': 'ATTACKER' if agent_type == self.ATTACKER else 'DEFENDER',
            'num_agents': num_agents,
            'gamma': gamma,
            'radius': radius,
            'attacker_count': attacker_count,
            'defender_count': defender_count
        }
        
        # Read Q-table
        q_table_size, = struct.unpack('<Q', data[pos:pos+8])
        pos += 8
        for _ in range(q_table_size):
            state, pos = self._read_joint_state(data, pos)
            inner_size, = struct.unpack('<Q', data[pos:pos+8])
            pos += 8
            
            inner_map = {}
            for _ in range(inner_size):
                action, pos = self._read_joint_action(data, pos)
                q_value, = struct.unpack('<d', data[pos:pos+8])
                pos += 8
                inner_map[action] = q_value
            
            self.q_table[state] = inner_map
        
        # Read N_s table
        ns_table_size, = struct.unpack('<Q', data[pos:pos+8])
        pos += 8
        for _ in range(ns_table_size):
            state, pos = self._read_joint_state(data, pos)
            count, = struct.unpack('<i', data[pos:pos+4])
            pos += 4
            self.ns_table[state] = count
        
        # Read N_s_a table
        nsa_table_size, = struct.unpack('<Q', data[pos:pos+8])
        pos += 8
        for _ in range(nsa_table_size):
            state, pos = self._read_joint_state(data, pos)
            inner_size, = struct.unpack('<Q', data[pos:pos+8])
            pos += 8
            
            inner_map = {}
            for _ in range(inner_size):
                action, pos = self._read_joint_action(data, pos)
                count, = struct.unpack('<i', data[pos:pos+4])
                pos += 4
                inner_map[action] = count
            
            self.nsa_table[state] = inner_map
    
    def _read_joint_state(self, data, pos):
        """Read a JointState from binary data."""
        size, = struct.unpack('<Q', data[pos:pos+8])
        pos += 8
        
        agents = []
        for _ in range(size):
            agent_id, = struct.unpack('<i', data[pos:pos+4])
            pos += 4
            agent_type, = struct.unpack('<B', data[pos:pos+1])
            pos += 1
            q, = struct.unpack('<i', data[pos:pos+4])
            pos += 4
            r, = struct.unpack('<i', data[pos:pos+4])
            pos += 4
            
            agents.append({
                'id': agent_id,
                'type': 'ATTACKER' if agent_type == self.ATTACKER else 'DEFENDER',
                'position': {'q': q, 'r': r}
            })
        
        # Convert to tuple for hashing
        state_tuple = tuple(
            (a['id'], a['type'], a['position']['q'], a['position']['r'])
            for a in agents
        )
        return state_tuple, pos
    
    def _read_joint_action(self, data, pos):
        """Read a JointAction from binary data."""
        size, = struct.unpack('<Q', data[pos:pos+8])
        pos += 8
        
        actions = []
        for _ in range(size):
            action, = struct.unpack('<B', data[pos:pos+1])
            pos += 1
            actions.append(action)
        
        return tuple(actions), pos
    
    def display(self):
        """Display checkpoint information in human-readable format."""
        print("=" * 60)
        print("Q-TABLE CHECKPOINT")
        print("=" * 60)
        print()
        
        print("METADATA:")
        print("-" * 60)
        for key, value in self.metadata.items():
            if key == 'timestamp':
                from datetime import datetime
                dt = datetime.fromtimestamp(value)
                print(f"  {key:20s}: {dt} ({value})")
            else:
                print(f"  {key:20s}: {value}")
        print()
        
        print("Q-TABLE STATISTICS:")
        print("-" * 60)
        print(f"  States:              {len(self.q_table)}")
        total_entries = sum(len(inner) for inner in self.q_table.values())
        print(f"  Total Q-values:      {total_entries}")
        
        if total_entries > 0:
            all_q_values = [q for inner in self.q_table.values() for q in inner.values()]
            print(f"  Min Q-value:         {min(all_q_values):.6f}")
            print(f"  Max Q-value:         {max(all_q_values):.6f}")
            print(f"  Mean Q-value:        {sum(all_q_values)/len(all_q_values):.6f}")
        print()
        
        print("VISIT COUNT STATISTICS:")
        print("-" * 60)
        print(f"  N_s entries:         {len(self.ns_table)}")
        print(f"  N_s_a states:        {len(self.nsa_table)}")
        total_nsa = sum(len(inner) for inner in self.nsa_table.values())
        print(f"  N_s_a total entries: {total_nsa}")
        
        if self.ns_table:
            print(f"  Max state visits:    {max(self.ns_table.values())}")
        print()
        
        print("=" * 60)
    
    def export_json(self, output_path):
        """Export checkpoint data to JSON format."""
        def convert_state(state_tuple):
            return [
                {
                    'id': s[0],
                    'type': s[1],
                    'position': {'q': s[2], 'r': s[3]}
                }
                for s in state_tuple
            ]
        
        def convert_action(action_tuple):
            return list(action_tuple)
        
        data = {
            'metadata': self.metadata,
            'q_table': {
                str(convert_state(state)): {
                    str(convert_action(action)): q_value
                    for action, q_value in inner.items()
                }
                for state, inner in self.q_table.items()
            },
            'ns_table': {
                str(convert_state(state)): count
                for state, count in self.ns_table.items()
            },
            'nsa_table': {
                str(convert_state(state)): {
                    str(convert_action(action)): count
                    for action, count in inner.items()
                }
                for state, inner in self.nsa_table.items()
            }
        }
        
        with open(output_path, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"Exported to {output_path}")


def main():
    """Main entry point for command-line usage."""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Parse and display Q-table checkpoint files from Perimeter-MG',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Display checkpoint metadata and statistics
  %(prog)s qtables/scenario_2a_1d_agent0_step1000.bin
  
  # Export Q-table data to JSON
  %(prog)s checkpoint.bin --json output.json
  
  # Process multiple checkpoints
  for f in qtables/*.bin; do %(prog)s "$f"; done

For more information, see docs/CHECKPOINT_USAGE.md
        """
    )
    
    parser.add_argument('checkpoint', help='Path to checkpoint binary file (.bin)')
    parser.add_argument('--json', dest='json_output', metavar='FILE',
                        help='Export Q-table data to JSON file')
    
    args = parser.parse_args()
    
    if not Path(args.checkpoint).exists():
        print(f"Error: File not found: {args.checkpoint}", file=sys.stderr)
        sys.exit(1)
    
    try:
        qtable_parser = QTableParser(args.checkpoint)
        qtable_parser.parse()
        qtable_parser.display()
        
        if args.json_output:
            qtable_parser.export_json(args.json_output)
    
    except Exception as e:
        print(f"Error parsing checkpoint: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
