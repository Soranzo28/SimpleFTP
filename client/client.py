import socket
from enum import IntEnum
import struct
from pathlib import Path
import zlib
from dataclasses import dataclass
import argparse

# =BHI255s


class msg_type(IntEnum):
    MSG_SEND = 1
    MSG_ACK = 2
    MSG_DATA = 3
    MSG_DONE = 4
    MSG_ERROR = 5


# HEADER CLASS
FMT = "=BHI255s"
HEADER_SIZE = struct.calcsize(FMT)


@dataclass
class Header:
    type: int
    payload_size: int
    checksum: int
    filename: bytes

    @classmethod
    def from_bytes(cls, data: bytes):
        type_, payload_size, checksum, filename = struct.unpack(FMT, data)
        return cls(type_, payload_size, checksum, filename.rstrip(b"\x00").decode())


# ERROR CLASSES


class CRCMismatchError(Exception):
    pass


class ConnectionError(OSError):
    pass


# MAIN CLASS


class SimpleFTP:
    def __init__(self, chunk_size=8192, send_retries=3):

        # public properties
        self.chunk_size = chunk_size
        self.send_retries = send_retries
        self.checksum = 0
        # private properties
        self.__socket = socket.socket()

    def connect(self, ip, port):

        self.__socket.connect((ip, port))

    def disconnect(self):
        if self.__socket:
            self.__socket.close()

    def send_file(self, filepath):
        p = Path(filepath)
        print(f"Sending file {p.name}")

        send_header = self._create_send_header(p.name)
        self.__socket.send(send_header)

        for chunk in self._chunkinize_file(filepath):
            self.checksum = self._calculate_chunk_checksum(chunk)
            data_packet = self._create_data_packet(chunk, self.checksum)
            self.__socket.send(data_packet)

            header = self._recv_response()

            if header.type == msg_type.MSG_ERROR:
                self._resend_packet(data_packet)
            elif header.type == msg_type.MSG_ACK:
                continue
            else:
                raise ConnectionError()

        done_header = self._create_done_header()
        self.__socket.send(done_header)
        print("File sent!")

    def _create_send_header(self, filename):
        header = struct.pack("=BHI255s", msg_type.MSG_SEND, 0, 0, filename.encode())
        return header

    def _create_data_packet(self, chunk, checksum):
        header = struct.pack("=BHI255s", msg_type.MSG_DATA, len(chunk), checksum, b"")
        packet = header + chunk
        return packet

    def _create_done_header(self):
        header = struct.pack("=BHI255s", msg_type.MSG_DONE, 0, self.checksum, b"")
        return header

    def _calculate_chunk_checksum(self, chunk):
        self.checksum = zlib.crc32(chunk, self.checksum) & 0xFFFFFFFF
        return self.checksum

    def _chunkinize_file(self, filepath):
        p = Path(filepath)

        if not p.exists():
            raise FileNotFoundError(f"File not found! {filepath}")
        if not p.is_file():
            raise ValueError(f"Path is not a file! {filepath}")

        with open(filepath, "rb") as f:
            while True:
                chunk = f.read(self.chunk_size)
                if not chunk:
                    return
                yield chunk

    def _recv_response(self):
        raw_header = self.__socket.recv(HEADER_SIZE)
        header = Header.from_bytes(raw_header)
        return header

    def _resend_packet(self, data_packet):
        print("CRM error detected, trying to send packet again...")
        for _ in range(self.send_retries):
            self.__socket.send(data_packet)
            header = self._recv_response()
            if header.type == msg_type.MSG_ACK:
                print("Message re-sended successfully!")
                return
        raise CRCMismatchError()


def main():

    parser = argparse.ArgumentParser(description="SimpleFTP Client")
    parser.add_argument("filepath", help="File to be sent")
    parser.add_argument("--ip", default="localhost", help="Server's IP")
    parser.add_argument("--port", type=int, default=9000, help="Server's PORT")

    args = parser.parse_args()

    ftp = SimpleFTP()
    ftp.connect(args.ip, args.port)
    ftp.send_file(args.filepath)
    ftp.disconnect()


if __name__ == "__main__":
    main()
