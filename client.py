import socket
import struct
import pyaudio
from io import BytesIO
import pygame
import datetime

HOST = "127.0.0.1"  # The server's hostname or IP address
PORT = 8080  # The port used by the server
audio_buffer = b""
keys = {
    pygame.K_x: 0,
    pygame.K_s: 1,
    pygame.K_SPACE: 2,
    pygame.K_RETURN: 3,
    pygame.K_UP: 4,
    pygame.K_DOWN: 5,
    pygame.K_LEFT: 6,
    pygame.K_RIGHT: 7,
    pygame.K_z: 8,
    pygame.K_a: 9,
    pygame.K_d: 10,
    pygame.K_c: 11,
    pygame.K_f: 12,
    pygame.K_v: 13,
    pygame.K_g: 14,
    pygame.K_b: 15,
}
# RETRO_DEVICE_ID_JOYPAD_B        0
# RETRO_DEVICE_ID_JOYPAD_Y        1
# RETRO_DEVICE_ID_JOYPAD_SELECT   2
# RETRO_DEVICE_ID_JOYPAD_START    3
# RETRO_DEVICE_ID_JOYPAD_UP       4
# RETRO_DEVICE_ID_JOYPAD_DOWN     5
# RETRO_DEVICE_ID_JOYPAD_LEFT     6
# RETRO_DEVICE_ID_JOYPAD_RIGHT    7
# RETRO_DEVICE_ID_JOYPAD_A        8
# RETRO_DEVICE_ID_JOYPAD_X        9
# RETRO_DEVICE_ID_JOYPAD_L       10
# RETRO_DEVICE_ID_JOYPAD_R       11
# RETRO_DEVICE_ID_JOYPAD_L2      12
# RETRO_DEVICE_ID_JOYPAD_R2      13
# RETRO_DEVICE_ID_JOYPAD_L3      14
# RETRO_DEVICE_ID_JOYPAD_R3      15

def callback(in_data, frame_count, time_info, status):
        # print("callback")
        global audio_buffer
        # print(in_data, frame_count, time_info, status)
        # data = wf.readframes(frame_count)
        # If len(data) is less than requested frame_count, PyAudio automatically
        # assumes the stream is finished, and the stream stops.
        new_data = audio_buffer[:frame_count*4]
        audio_buffer = audio_buffer[frame_count*4:]
        
        if len(new_data) < frame_count*4:
            new_data += b"\00"*(frame_count*4)
        # print(x)
        return (new_data, pyaudio.paContinue)

# p = pyaudio.PyAudio()
# stream = p.open(
#     format=pyaudio.paInt16,
#     channels=2,
#     rate=int(32040),
#     output=True,
#     # stream_callback=callback
# )

def receive_data(sock):
    cmd = struct.unpack("h", s.recv(2))[0]
    buff_size = struct.unpack("i", s.recv(4))[0]
    data = b""
    while len(data) < buff_size:
        data += sock.recv(buff_size-len(data))
    return cmd, buff_size, data

pygame.init()
canvas = pygame.display.set_mode((500, 500))
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    cmd = 1
    # s.sendall(struct.pack("h", cmd))
    # s.sendall(struct.pack("h", cmd))
    # s.sendall(struct.pack("h", cmd))
    counter = 0
    # stream.start_stream()
    while(True):
        # buff_size = struct.unpack("i", s.recv(4))[0]
        # print(buff_size)
        # data = s.recv(buff_size)
        # print(1, len(data))
        # if len(data) < buff_size:
        #     data += s.recv(buff_size-len(data))
        # print(2, len(data))
        # buffer = struct.unpack("h"*(buff_size//2), data)
        # print(buffer)
        # data = b"".join([data[i].to_bytes(2, "little", signed=True) for i in range(frames)])
        # print("AUDIO", frames, stream.get_output_latency())
        cmd, buff_size, data = receive_data(s)
        counter += 1
        # print(datetime.datetime.now(), counter, cmd, buff_size, len(data))
        if cmd == 1:
            stream.write(data)
            # audio_buffer += data
            pass
        elif cmd == 2:
            b = BytesIO(data)
            recvsurface = pygame.image.load(b)
            canvas.blit(recvsurface, (0,0))
            pygame.display.update()
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                raise SystemExit
        
        pressed = pygame.key.get_pressed()
        # for key in keys:
        #     pressed_or_not = 1 if pressed[key] else 2
        #     s.sendall(struct.pack("h", pressed_or_not))
        #     s.sendall(struct.pack("h", 0))
        #     s.sendall(struct.pack("h", keys[key]))
        #     print("KEY SENT: {} | PRESSED: {}".format(keys[key], pressed_or_not))
            
        # for event in pygame.event.get():
        #     if event.type == pygame.KEYDOWN and event.key in keys:
        #         s.sendall(struct.pack("h", 1))
        #         s.sendall(struct.pack("h", 0))
        #         s.sendall(struct.pack("h", keys[event.key]))
        #         print("KEY SENT: {}".format(keys[event.key]))