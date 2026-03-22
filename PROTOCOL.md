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
| `checksum` | `uint32_t` | 4 bytes | CRC32/ISO-HDLC checksum of the chunk payload |
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
| `MSG_DONE` | `0x04` | Client → Server | Signals end of transfer |
| `MSG_ERROR` | `0x05` | Both | Signals an error or requests retransmission |

---

## Transfer Flow

```
Client                          Server
  |                                |
  |---[ MSG_SEND | filename ]----->|  Client announces filename
  |<--[ MSG_ACK  | filename ]------|  Server creates file, confirms
  |                                |
  |---[ MSG_DATA | chunk 1  ]----->|  Client sends chunk + CRC32
  |<--[ MSG_ACK             ]------|  Server verifies CRC32, writes chunk
  |                                |
  |---[ MSG_DATA | chunk 2  ]----->|
  |<--[ MSG_ACK             ]------|
  |           ...                  |
  |---[ MSG_DATA | chunk N  ]----->|  Last chunk (may be smaller than CHUNK_SIZE)
  |<--[ MSG_ACK             ]------|
  |                                |
  |---[ MSG_DONE            ]----->|  Client signals end of transfer
  |                                |  Server closes the file
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

The client retries up to **3 times** (`MAX_RESEND_TRIES`). If all retries fail, a `CRCMismatchError` is raised and the transfer is aborted.

---

## Chunk Size

| Constant | Value | Description |
|----------|-------|-------------|
| `CHUNK_SIZE` | `8192` (8KB) | Maximum payload size per `MSG_DATA` message |
| `MAX_RESEND_TRIES` | `3` | Maximum retransmission attempts per chunk |

The last chunk of a file may be smaller than `CHUNK_SIZE`. The `payload_size` field in the header always reflects the actual size of the chunk being sent.

---

## Checksum

Each `MSG_DATA` packet includes a **CRC32/ISO-HDLC** checksum of its payload, computed independently per chunk.

**C (server):**
```c
uLong crc = crc32(0L, Z_NULL, 0);
crc = crc32(crc, (const Bytef *)buffer, bytes_read);
```

**Python (client):**
```python
checksum = zlib.crc32(chunk) & 0xFFFFFFFF
```

Both use the same CRC32/ISO-HDLC variant — the same algorithm used by ZIP, PNG, and Ethernet.

---

## Notes

- `MSG_DATA` headers have an empty `filename` field — the filename is only used in `MSG_SEND` and `MSG_ACK`.
- The server does not validate the `filename` field in `MSG_DATA` or `MSG_DONE`.
- The connection is closed by the server after receiving `MSG_DONE` or on unrecoverable error.
- Only one file transfer per TCP connection is supported.
