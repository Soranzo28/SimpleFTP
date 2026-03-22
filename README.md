# SimpleFTP

A lightweight file transfer tool built on a custom binary protocol over TCP, implemented with a C server and Python client.

## Project Structure

```
SimpleFTP/
├── server/
│   ├── server.c        # Main server logic and entry point
│   └── server.h        # Args struct definition
├── protocol/
│   ├── protocol.c      # Protocol handling — headers, file writing, CRC32
│   └── protocol.h      # Protocol constants, Header struct, function declarations
├── net/
│   ├── net.c           # TCP socket creation and connection handling
│   └── net.h           # sock_fd typedef, New_connection_info struct
├── helpers/
│   ├── helpers.c       # IP conversion utilities
│   └── helpers.h       # DEBUG_PRINT macro, ip_octetes struct
└── client.py           # Python client (SimpleFTP class + CLI)
```

## Building

```bash
gcc server/server.c protocol/protocol.c net/net.c helpers/helpers.c -lz -o server
```

## Usage

### Server

```bash
./server [--debug] [--ip IP] [--port PORT] [--sp SAVE_PATH]
```

| Flag | Default | Description |
|------|---------|-------------|
| `--debug` / `-d` | off | Enable verbose debug output |
| `--ip` / `-i` | `0.0.0.0` | IP to bind the server |
| `--port` / `-p` | `9000` | Port to listen on |
| `--sp` / `-s` | `.` | Directory to save received files |

**Examples:**

```bash
# Start server on default port, save files to /tmp
./server --sp /tmp

# Start with debug mode on port 8000
./server --debug --port 8000 --sp /tmp/received
```

### Client

```bash
python3 client.py <filepath> [--ip IP] [--port PORT]
```

| Argument | Default | Description |
|----------|---------|-------------|
| `filepath` | required | Path to the file to send |
| `--ip` | `localhost` | Server IP |
| `--port` | `9000` | Server port |

**Examples:**

```bash
# Send a file to local server
python3 client.py /home/user/photo.png

# Send to a remote server
python3 client.py /home/user/backup.tar.gz --ip 192.168.0.10 --port 9000
```

## Dependencies

### Server (C)
- `zlib` — CRC32 checksum calculation

```bash
# Ubuntu/Debian
sudo apt install zlib1g-dev
```

### Client (Python)
- Python 3.8+
- Standard library only (`socket`, `struct`, `zlib`, `pathlib`, `argparse`)

## Features

- Custom binary protocol over TCP
- CRC32 integrity verification per chunk
- Automatic chunk retransmission on CRC mismatch (up to 3 retries)
- Configurable chunk size (default 8KB)
- Debug mode with detailed per-chunk logging
- Supports files of any size

## Known Limitations

These are known limitations of the current v1 implementation:

**No end-to-end integrity check** — CRC32 is verified per chunk, but there is no checksum of the complete file in `MSG_DONE`. If the connection drops mid-transfer, the incomplete file remains on disk with no indication it is corrupted. A future version should include a full-file checksum in `MSG_DONE` and have the server delete partial files on failure.

**Silent file overwrite** — if the server receives a file with the same name as an existing one, it overwrites it silently. A future version could append a timestamp to the filename or prompt before overwriting.

**Single-threaded** — the server handles one client at a time, blocking on `accept()` until the current transfer finishes. Concurrent transfers would require `fork()` or `pthread` per connection.

**One file per connection** — each TCP connection transfers exactly one file. The connection is closed after `MSG_DONE`. Sending multiple files requires reconnecting for each one.

**No authentication** — any client that can reach the server's IP and port can send files. There is no access control or encryption.
