import tkinter as tk
from tkinter import ttk
import serial.tools.list_ports
from threading import Thread
import esptool
from ttkthemes import ThemedStyle
import os
import sys

def resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    base_path = getattr(sys, '_MEIPASS', os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base_path, relative_path)

def esptoolnonblock(args, func):
    global start
    start = True
    thread_list = []
    startesptool = True
    esptool.main(args)

def esp32f():
    selected_port = port_var.get()
    bootloader_path = resource_path('bootloader.bin')
    partitions_path = resource_path('partitions.bin')
    bootapp0_bin = resource_path('boot_app0.bin')
    firmware_path = resource_path('firmware.bin')
    
    command = ['--port', str(selected_port), '--baud', '460800', '--before', 'default_reset', '--after', 'hard_reset', 'write_flash', '-z', '--flash_mode', 'dio', '--flash_freq', '40m', '--flash_size', '4MB', '0x1000', bootloader_path, '0x8000', partitions_path, '0xe000', bootapp0_bin, '0x10000', firmware_path]
    esptoolnonblock(command, esp32f)

def refresh_ports():
    port_combobox['values'] = [port.device for port in serial.tools.list_ports.comports()]

# Create the main window
root = tk.Tk()
root.title("ESPTool Interface")

# Apply dark theme
style = ThemedStyle(root)
style.set_theme("equilux")

# Configure the root window
root.geometry("400x300")
root.resizable(False, False)
root.configure(bg='#262626')  # Set background color to grey

# Create and pack widgets
port_var = tk.StringVar()
port_combobox = ttk.Combobox(root, textvariable=port_var, state='readonly', font=('Arial', 12))
port_combobox.pack(pady=10, padx=20, fill=tk.X)

refresh_button = ttk.Button(root, text="Refresh Ports", command=refresh_ports, style='Dark.TButton')
refresh_button.pack(pady=10, padx=20, fill=tk.X)

run_button = ttk.Button(root, text="Run ESP32F", command=esp32f, style='Dark.TButton')
run_button.pack(pady=10, padx=20, fill=tk.X)

# Initialize ports dropdown
refresh_ports()

# Start the Tkinter event loop
root.mainloop()
