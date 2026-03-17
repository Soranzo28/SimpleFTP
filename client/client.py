import socket
from enum import IntEnum
import struct
from crc import Crc32, Calculator

s = socket.socket()
s.connect(("localhost", 9000))

CHUNK_SIZE = 8192


class msg_type(IntEnum):
    MSG_SEND = 0x01
    MSG_ACK = 0x02
    MSG_DATA = 0x03
    MSG_DONE = 0x04
    MSG_ERROR = 0x05


def create_send_header(filename):
    header = struct.pack("=BHI255s", msg_type.MSG_SEND, 0, 0, filename.encode())
    return header


def create_data_packet(checksum, chunk, size):
    header = struct.pack("=BHI255s", msg_type.MSG_DATA, size, checksum, b"")
    packet = header + chunk
    return packet


def create_done_header():
    header = struct.pack("=BHI255s", msg_type.MSG_DONE, 0, 0, b"")
    return header


s.send(create_send_header("w_berserk.png"))
print("Sent send header!")

f = open("/home/soranzo/Midia/Pictures/w_berserk.png", "rb")


while True:
    chunk = f.read(CHUNK_SIZE)
    if not chunk:
        break
    calculator = Calculator(Crc32.CRC32)
    chunk_checksum = calculator.checksum(chunk)
    data_packet = create_data_packet(chunk_checksum, chunk, len(chunk))
    s.send(data_packet)
    print("Sent data packet!")
    response = s.recv(struct.calcsize("=BHI255s"))
    r_msg_type, payload_size, checksum, filename = struct.unpack("=BHI255s", response)
    print(f"Ack: {r_msg_type}")
    if r_msg_type != msg_type.MSG_ACK:
        print("ERROR!")
        break


s.send(create_done_header())
print("Done sending file!")
