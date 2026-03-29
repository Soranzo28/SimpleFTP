# SimpleFTP Protocol Specification

## Overview

SimpleFTP uses a custom binary protocol over TCP. Every message consists of a fixed-size **Header** (262 bytes) optionally followed by a **payload** (chunk data).

---

## Header Structure

```
+--------+---------------+-------------------+---------------------+
| type   | payload_size  |     checksum      |      filename       |
| 1 byte |    2 bytes    |      4 bytes      |      255 bytes      |
+--------+---------------+-------------------+---------------------+
```

| Field | Type | Size | Description |
|-------|------|------|-------------|
| `type` | `uint8_t` | 1 byte | Message type identifier |
| `payload_size` | `uint16_t` | 2 bytes | Size of the payload following the header (0 if none) |
| `checksum` | `uint32_t` | 4 bytes | CRC32/ISO-HDLC checksum — cumulative over all chunks for `MSG_DATA`; full-file checksum for `MSG_DONE` |
| `filename` | `char[255]` | 255 bytes | Null-terminated filename, zero-padded |

**Total header size: 262 bytes**

The struct is packed (`__attribute__((packed))`) — no padding between fields.

Binary layout uses native byte order (`=` in Python's `struct` format: `=BHI255s`).

---

## Message Types

| Name | Value | Direction | Description |
|------|-------|-----------|-------------|
| `MSG_SEND` | `0x01` | Client → Server | Initiates a file transfer |
| `MSG_ACK` | `0x02` | Server → Client | Acknowledges a received message |
| `MSG_DATA` | `0x03` | Client → Server | Sends a file chunk |
| `MSG_DONE` | `0x04` | Client → Server | Signals end of transfer, carries full-file CRC32 |
| `MSG_ERROR` | `0x05` | Both | Signals an error or requests retransmission |

---

## Transfer Flow

```
Client                          Server
  |                                |
  |---[ MSG_SEND | filename ]----->|  Client announces filename
  |<--[ MSG_ACK  | filename ]------|  Server creates file, confirms ready
  |                                |
  |---[ MSG_DATA | chunk 1  ]----->|  Client sends chunk + cumulative CRC32
  |<--[ MSG_ACK             ]------|  Server verifies CRC32, writes chunk
  |                                |
  |---[ MSG_DATA | chunk 2  ]----->|
  |<--[ MSG_ACK             ]------|
  |           ...                  |
  |---[ MSG_DATA | chunk N  ]----->|  Last chunk (may be smaller than CHUNK_SIZE)
  |<--[ MSG_ACK             ]------|
  |                                |
  |---[ MSG_DONE | full CRC ]----->|  Client signals end of transfer,
  |                                |  carries the final cumulative CRC32
  |                                |  Server validates full-file checksum,
  |                                |  deletes file on mismatch, closes connection
```

---

## Error Handling & Retransmission

If the server detects a CRC32 mismatch on a received chunk, it replies with `MSG_ERROR` instead of `MSG_ACK`.

```
Client                          Server
  |                                |
  |---[ MSG_DATA | chunk N  ]----->|  CRC mismatch detected
  |<--[ MSG_ERROR           ]------|  Server requests retransmission
  |                                |
  |---[ MSG_DATA | chunk N  ]----->|  Client resends the same chunk
  |<--[ MSG_ACK             ]------|  CRC matches, transfer continues
```

The server retries up to **`MAX_RESEND_TRIES`** times per chunk. The client also retries up to **`send_retries`** times (default 3). If all retries fail, a `CRCMismatchError` is raised on the client and the transfer is aborted.

---

## Chunk Size

| Constant | Value | Description |
|----------|-------|-------------|
| `CHUNK_SIZE` | `8192` (8KB) | Maximum payload size per `MSG_DATA` message |
| `MAX_RESEND_TRIES` | `3` | Maximum retransmission attempts per chunk (server-side) |

The last chunk of a file may be smaller than `CHUNK_SIZE`. The `payload_size` field in the header always reflects the actual size of the chunk being sent.

---

## Checksum

SimpleFTP uses **CRC32/ISO-HDLC** (the same variant used by ZIP, PNG, and Ethernet) for integrity verification at two levels:

### Per-chunk (during transfer)

The CRC32 is computed **cumulatively** across all chunks sent so far. Each `MSG_DATA` header carries the running checksum up to and including that chunk.

**C (server):**
```c
// Initialized once at the start of the transfer
uLong crc = crc32(0L, Z_NULL, 0);

// Updated for each chunk received
crc = crc32(crc, (const Bytef *)buffer, bytes_read);
```

**Python (client):**
```python
# Initialized to 0 before the transfer
checksum = 0

# Updated for each chunk sent
checksum = zlib.crc32(chunk, checksum) & 0xFFFFFFFF
```

### Full-file (MSG_DONE)

After all chunks are sent, the client includes the **final cumulative CRC32** in the `MSG_DONE` header's `checksum` field. The server compares this against its own accumulated CRC32. If they do not match, the server deletes the incomplete/corrupt file.

---

## Notes

- `MSG_DATA` headers have an empty `filename` field — the filename is only used in `MSG_SEND` and `MSG_ACK`.
- The server does not validate the `filename` field in `MSG_DATA` or `MSG_DONE`.
- The connection is closed by the server after receiving `MSG_DONE` or on unrecoverable error.
- Only one file transfer per TCP connection is supported.
- On a full-file CRC32 mismatch after `MSG_DONE`, the server automatically deletes the written file to avoid leaving corrupt data on disk.
