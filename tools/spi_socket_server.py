#!/usr/bin/env python3
"""
SPI HAL Socket Server - TLE92104 Simulation
A TCP socket server simulating the Infineon TLE92104 4-channel high-side switch.

Simulates:
 - Register reads/writes via SPI 16-bit frames
 - Device ID (0x5A) in DEVID register
 - Watchdog register tracking
 - Diagnostic register (no faults by default)

SPI Frame format (16-bit):
  [CMD(2) | ADDR(4) | DATA(8) | PARITY(1) | RESERVED(1)]
  CMD: 00=Read, 01=Write
  ADDR: 4-bit register address
  DATA: 8-bit data
  PARITY: even parity over bits 15:2

Protocol over socket (per HAL socket implementation):
  TX Header: [msg_type(1) | device_id(1) | data_length(2) | sequence(4)]  = 8 bytes
  TX Payload: data_length bytes
  RX Header: [0x80(1) | device_id(1) | data_length(2) | sequence(4)]  = 8 bytes
  RX Payload: data_length bytes

Usage:
    python spi_socket_server.py [--host HOST] [--port PORT]

Author: EswPla Team
Date: 2026-02-21
"""

import socket
import struct
import argparse
import threading
import time
import sys
from enum import IntEnum

class SpiMessageType(IntEnum):
    """SPI protocol message types"""
    INIT         = 0x01
    DEINIT       = 0x02
    TRANSFER     = 0x03
    SEND         = 0x04
    RECEIVE      = 0x05
    SET_CONFIG   = 0x06
    GET_STATUS   = 0x07
    RESPONSE     = 0x80


class TLE92104Simulator:
    """Simulates the TLE92104 register file and SPI behavior"""

    # Register addresses
    REG_CTRL1 = 0x00
    REG_CTRL2 = 0x01
    REG_CTRL3 = 0x02
    REG_CFG   = 0x03
    REG_DIAG  = 0x04
    REG_WDG   = 0x05
    REG_ICR   = 0x06
    REG_HWCR  = 0x07
    REG_DEVID = 0x08

    DEVICE_ID = 0x5A

    # SPI frame bit positions
    CMD_SHIFT  = 14
    ADDR_SHIFT = 10
    DATA_SHIFT = 2
    PARITY_BIT = 1

    def __init__(self):
        """Initialize with default register values"""
        self.registers = {
            self.REG_CTRL1: 0x00,
            self.REG_CTRL2: 0x00,
            self.REG_CTRL3: 0x00,
            self.REG_CFG:   0x00,
            self.REG_DIAG:  0x00,
            self.REG_WDG:   0x00,
            self.REG_ICR:   0x00,
            self.REG_HWCR:  0x00,
            self.REG_DEVID: self.DEVICE_ID,
        }
        self.wdg_count = 0
        self.last_response_data = 0x00
        self.last_response_addr = 0x00
        print(f"[TLE92104-SIM] Initialized. Device ID=0x{self.DEVICE_ID:02X}")

    def process_spi_frame(self, frame_16bit):
        """
        Process a 16-bit SPI frame and return 16-bit response.
        Response contains data from the previous transaction (pipeline behavior).
        """
        cmd  = (frame_16bit >> self.CMD_SHIFT) & 0x03
        addr = (frame_16bit >> self.ADDR_SHIFT) & 0x0F
        data = (frame_16bit >> self.DATA_SHIFT) & 0xFF

        response_data = self.last_response_data
        response_addr = self.last_response_addr

        if cmd == 0x00:  # READ
            self.last_response_data = self.registers.get(addr, 0x00)
            self.last_response_addr = addr
            print(f"[TLE92104-SIM] READ  reg[0x{addr:X}] -> 0x{self.last_response_data:02X}")
        elif cmd == 0x01:  # WRITE
            if addr != self.REG_DEVID:
                old_val = self.registers.get(addr, 0x00)
                self.registers[addr] = data
                print(f"[TLE92104-SIM] WRITE reg[0x{addr:X}] = 0x{data:02X} (was 0x{old_val:02X})")
                if addr == self.REG_WDG:
                    self.wdg_count += 1
                    if self.wdg_count % 10 == 0:
                        print(f"[TLE92104-SIM] Watchdog serviced {self.wdg_count} times")
            else:
                print(f"[TLE92104-SIM] WRITE to read-only DEVID register ignored")
            self.last_response_data = self.registers.get(addr, 0x00)
            self.last_response_addr = addr
        else:
            print(f"[TLE92104-SIM] Unknown CMD=0x{cmd:X}")
            self.last_response_data = 0x00
            self.last_response_addr = 0x00

        # Build response frame
        resp_frame = ((response_addr & 0x0F) << self.ADDR_SHIFT) | \
                     ((response_data & 0xFF) << self.DATA_SHIFT)
        # Calculate even parity
        temp = (resp_frame >> 2) & 0x3FFF
        parity = 0
        for _ in range(14):
            parity ^= (temp & 1)
            temp >>= 1
        resp_frame |= (parity << self.PARITY_BIT)

        return resp_frame


class SpiSocketServer:
    """SPI HAL Socket Server with TLE92104 simulation"""

    def __init__(self, host='127.0.0.1', port=9000):
        self.host = host
        self.port = port
        self.server_socket = None
        self.client_socket = None
        self.running = False
        self.tle92104 = TLE92104Simulator()
        self.spi_devices = {}

    def start(self):
        """Start the socket server"""
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(1)
            self.running = True
            print(f"[SPI-SERVER] Listening on {self.host}:{self.port}")

            while self.running:
                print("[SPI-SERVER] Waiting for connection...")
                self.client_socket, client_addr = self.server_socket.accept()
                print(f"[SPI-SERVER] Client connected from {client_addr}")

                client_thread = threading.Thread(target=self.handle_client)
                client_thread.start()
                client_thread.join()

        except KeyboardInterrupt:
            print("\n[SPI-SERVER] Shutting down...")
        except Exception as e:
            print(f"[SPI-SERVER] ERROR: {e}")
        finally:
            self.stop()

    def stop(self):
        """Stop the server"""
        self.running = False
        if self.client_socket:
            self.client_socket.close()
        if self.server_socket:
            self.server_socket.close()
        print("[SPI-SERVER] Server stopped")

    def handle_client(self):
        """Handle client connection"""
        try:
            while self.running:
                header_data = self.receive_all(8)
                if not header_data:
                    break

                msg_type, device_id, data_length, sequence = struct.unpack('<BBHI', header_data)

                payload = None
                if data_length > 0:
                    payload = self.receive_all(data_length)
                    if not payload:
                        break

                response = self.process_message(msg_type, device_id, payload)

                if response is not None:
                    self.send_response(msg_type, device_id, sequence, response)

        except Exception as e:
            print(f"[SPI-SERVER] Client error: {e}")
        finally:
            if self.client_socket:
                self.client_socket.close()
                self.client_socket = None
            print("[SPI-SERVER] Client disconnected")

    def receive_all(self, length):
        """Receive exact number of bytes"""
        data = b''
        while len(data) < length:
            chunk = self.client_socket.recv(length - len(data))
            if not chunk:
                return None
            data += chunk
        return data

    def send_response(self, msg_type, device_id, sequence, data):
        """Send response message"""
        response_type = SpiMessageType.RESPONSE
        data_length = len(data) if data else 0
        header = struct.pack('<BBHI', response_type, device_id, data_length, sequence)
        self.client_socket.sendall(header)
        if data:
            self.client_socket.sendall(data)

    def process_message(self, msg_type, device_id, payload):
        """Process received message and generate response"""

        if msg_type == SpiMessageType.INIT:
            if payload and len(payload) >= 7:
                baudrate, mode, bit_order, data_bits = struct.unpack('<IBBB', payload[:7])
                self.spi_devices[device_id] = {
                    'baudrate': baudrate,
                    'mode': mode,
                    'bit_order': bit_order,
                    'data_bits': data_bits,
                    'initialized': True
                }
                print(f"[SPI-SERVER] Device {device_id} initialized: "
                      f"{baudrate}Hz, mode={mode}, {data_bits}-bit")
            self.tle92104 = TLE92104Simulator()
            return b''

        elif msg_type == SpiMessageType.DEINIT:
            if device_id in self.spi_devices:
                del self.spi_devices[device_id]
                print(f"[SPI-SERVER] Device {device_id} deinitialized")
            return b''

        elif msg_type == SpiMessageType.TRANSFER:
            return self.process_spi_transfer(payload)

        elif msg_type == SpiMessageType.SEND:
            if payload:
                self.process_spi_transfer(payload)
            return b''

        elif msg_type == SpiMessageType.RECEIVE:
            if payload and len(payload) >= 2:
                requested_length = struct.unpack('>H', payload[:2])[0]
                return bytes(requested_length)
            return b''

        elif msg_type == SpiMessageType.SET_CONFIG:
            if payload and len(payload) >= 7:
                baudrate, mode, bit_order, data_bits = struct.unpack('<IBBB', payload[:7])
                if device_id in self.spi_devices:
                    self.spi_devices[device_id].update({
                        'baudrate': baudrate,
                        'mode': mode,
                        'bit_order': bit_order,
                        'data_bits': data_bits
                    })
                    print(f"[SPI-SERVER] Device {device_id} reconfigured")
            return b''

        elif msg_type == SpiMessageType.GET_STATUS:
            return struct.pack('<BB', 1, 0)

        else:
            print(f"[SPI-SERVER] Unknown message type: {hex(msg_type)}")
            return b''

    def process_spi_transfer(self, payload):
        """
        Process SPI transfer payload through TLE92104 simulator.
        Payload contains raw SPI bytes (2 bytes = 1 x 16-bit frame).
        """
        if not payload or len(payload) < 2:
            return payload if payload else b''

        response = bytearray()
        for i in range(0, len(payload) - 1, 2):
            tx_frame = (payload[i] << 8) | payload[i + 1]
            rx_frame = self.tle92104.process_spi_frame(tx_frame)
            response.append((rx_frame >> 8) & 0xFF)
            response.append(rx_frame & 0xFF)

        if len(payload) % 2 != 0:
            response.append(0x00)

        return bytes(response)


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='SPI HAL Socket Server - TLE92104 Simulation')
    parser.add_argument('--host', default='127.0.0.1',
                        help='Server host address (default: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=9000,
                        help='Server port (default: 9000)')

    args = parser.parse_args()

    print("=" * 60)
    print("SPI HAL Socket Server - TLE92104 Simulation")
    print("=" * 60)
    print(f"Host: {args.host}")
    print(f"Port: {args.port}")
    print(f"Simulated device: TLE92104 (ID=0x5A)")
    print("Press Ctrl+C to stop")
    print("=" * 60)

    server = SpiSocketServer(host=args.host, port=args.port)
    server.start()


if __name__ == '__main__':
    main()
