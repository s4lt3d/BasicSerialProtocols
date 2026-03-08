# Basic Serial Protocols

> Cross-platform Fourien protocol implementation for synchronized microcontroller-to-host serial communication with callback-based message handling.

---

## Key Docs

- **C++ Implementation:** `FourienComm.h` — Arduino/embedded systems
- **Python Implementation:** `FourienComm.py`, `FourienProtocol.py` — Host-side communication
- **C# Implementation:** `FourienProtocol.cs` — .NET/Windows integration
- **Test Suite:** `hardware_test.py`, `FourienProtocolTest.ino` — Examples and testing

---

## Overview

The Fourien protocol standardizes serial communication between microcontrollers and host systems. It provides reliable, synchronized packet transmission using preamble-based framing, circular buffering, and callback-driven message handling. Implementations available for embedded systems (Arduino/C++), Python hosts, and .NET environments.

---

## Features

- **Multi-Language Support** — C++, Python, and C# implementations for cross-platform communication
- **Preamble-Synchronized Framing** — 4x 0x55 bytes ensure packet alignment and recovery from corruption
- **Circular Buffer Architecture** — 2048-byte buffer with efficient memory management
- **Message Types** — ASCII text, version queries, memory read/write operations, burst transfers
- **Callback System** — Register handlers for specific message types with asynchronous processing
- **Memory Access** — Direct read/write to microcontroller memory with address-based addressing
- **Burst Transfers** — Efficient multi-value transfers for sensor data or telemetry

---

## Usage

### Python Example

```python
import FourienComm as fc
import numpy as np
import matplotlib.pyplot as plt

# Create connection
comm = fc.FourienComm("COM4")

# Register callbacks
comm.set_ascii_message_callback(lambda msg, time: print(f"Message: {msg}"))
comm.set_memory_read_callback(lambda addr, data, time: print(f"Read {addr}: {data}"))

# Send commands
comm.get_version()
comm.write_memory(1, 12345)
comm.read_memory(1)
comm.read_memory(17)  # Burst read

comm.close()
```

### Message Protocol

- **Preamble:** 4x 0x55 bytes to synchronize packet start
- **Message Types:**
  - `'A'` — ASCII message
  - `'V'` — Version query
  - `0x10` — Memory read request
  - `0x20` — Memory write request

---

## Architecture

### Microcontroller Side (Arduino)

- `serviceIncomingSerial()` — Monitors serial port and fills circular buffer
- `parseBuffer()` — Extracts complete packets and dispatches to handlers
- `sendPreamble()`, `sendMemoryMessage()`, `sendAsciiMessage()` — Format and transmit responses
- Memory callbacks — User-defined handlers for read/write operations
- Circular buffer with automatic overflow handling

### Host Side (Python/C#)

- Connection management over serial port
- Protocol encoding/decoding for all message types
- Callback dispatch for incoming messages
- Utilities for common operations (read, write, version, burst transfers)
- Matplotlib integration for data visualization (Python)

---

## Dependencies

- **Arduino:** [CircularBuffer](https://www.arduino.cc/reference/en/libraries/circularbuffer/) library (v1.1.3 by Agileware)
- **Python:** `numpy`, `matplotlib` (for test utilities)

---

## Design Pattern

Uses callback-based event dispatch:
1. Serial data arrives → Circular buffer stores bytes
2. Complete packet detected (preamble + payload) → `parseBuffer()` extracts message
3. Message type determined → Appropriate callback invoked with data
4. Host/device responds asynchronously via callback handlers

This decouples message reception from processing, allowing the serial handler to remain responsive.

---

## License

Copyright © Walter Gordy
