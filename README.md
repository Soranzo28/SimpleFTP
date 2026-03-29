# SimpleFTP

A lightweight file transfer tool built on a custom binary protocol over TCP, implemented with a C server and Python client.

## Project Structure

```
SimpleFTP/
├── server/
│   ├── server.c        # Main server loop, CLI argument parsing, entry point
│   └── server.h        # Args struct definition
├── protocol/
│   ├── protocol.c      # Protocol handling — headers, file writing, CRC32 validation
│   └── protocol.h      # Protocol constants, message types, function declarations
├── net/
│   ├── net.c           # TCP socket creation and connection acceptance
│   └── net.h           # sock_fd typedef, function declarations
├── helpers/
│   ├── helpers.c       # Path handling, CRC32 helpers, header creation/sending, data reading
│   └── helpers.h       # DEBUG_PRINT macro, function declarations
├── types.h             # Shared types: Header, Args, ConnectionContext, ip_octetes
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
- CRC32 integrity verification per chunk (cumulative)
- End-to-end integrity check: full-file CRC32 sent in `MSG_DONE`, server deletes file on mismatch
- Automatic chunk retransmission on CRC mismatch (up to 3 retries)
- Configurable chunk size (default 8KB)
- Debug mode with detailed per-chunk logging
- Supports files of any size
- Filename sanitization on the server (strips path separators and hidden file indicators)

## Known Limitations

These are known limitations of the current implementation:

**Silent file overwrite** — if the server receives a file with the same name as an existing one, it overwrites it silently. A future version could append a timestamp to the filename or prompt before overwriting.

**Single-threaded** — the server handles one client at a time, blocking on `accept()` until the current transfer finishes. Concurrent transfers would require `fork()` or `pthread` per connection.

**One file per connection** — each TCP connection transfers exactly one file. The connection is closed after `MSG_DONE`. Sending multiple files requires reconnecting for each one.

**No authentication** — any client that can reach the server's IP and port can send files. There is no access control or encryption.
