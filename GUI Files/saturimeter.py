import serial
from serial.tools.list_ports import comports

from kivy.app import App
from kivy.lang import Builder
from kivy.properties import ObjectProperty
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.gridlayout import GridLayout
from kivy.uix.screenmanager import Screen, ScreenManager
from kivy.clock import Clock

class ConnectionWindow(Screen):
    connection_button=ObjectProperty(None)
    spinner_serial=ObjectProperty(None)

    def __init__(self, **kwargs):
        super(ConnectionWindow, self).__init__(**kwargs)
        self.connection_status = False

    def on_spinner_serial(self, instance, value):
        ports = [port.device for port in comports()]
        self.spinner_serial.values = ports

    def button_pressed(self):
        portname=self.spinner_serial.text  
        global ser
        ser=serial.Serial(port=portname,baudrate=9600) 
        if(ser.is_open):
            sm.current="menuwindow"

class MenuWindow(Screen):
    connection_label=ObjectProperty(None)
    connection_button=ObjectProperty(None)

    global ser
    
    def celsius_pressed(self):
        ser.write(bytes(b'C'))
        
    def farheneit_pressed(self):
        ser.write(bytes(b'F'))

    def config_pressed(self):
        ser.write(bytes(b'S'))
        sm.current="settingswindow"

class SettingsWindow(Screen):

    global ser
    value_slider = ObjectProperty(None)
    connection_spinner=ObjectProperty(None)
    connection_button=ObjectProperty(None)

    def timerstart(self):
        global event
        event=Clock.schedule_once(self.timeout, 5)

    def timeout(self,dt):
        sm.current="menuwindow"
    
    def go_back(self):
        global event
        Clock.unschedule(event)

        ser.write(bytes(b'B'))
        sm.current="menuwindow"        
    
    def SpO2_sample_rate_select(self, text):
        global event
        Clock.unschedule(event)
        event=Clock.schedule_once(self.timeout, 5)

        if text=='50': 
            ser.write(serial.to_bytes([0xA0,0x00,0xC0])) 
        elif text=='100': 
            ser.write(serial.to_bytes([0xA0,0x04,0xC0])) 
        elif text=='200': 
            ser.write(serial.to_bytes([0xA0,0x08,0xC0])) 
        elif text=='400': 
            ser.write(serial.to_bytes([0xA0,0x0C,0xC0])) 

    def LED_pulsewidth_select(self, text):
        global event
        Clock.unschedule(event)
        event=Clock.schedule_once(self.timeout, 5)

        if text=='15': 
            ser.write(serial.to_bytes([0xA0,0x00,0xC1])) 
        elif text=='16': 
            ser.write(serial.to_bytes([0xA0,0x01,0xC1])) 
        elif text=='17': 
            ser.write(serial.to_bytes([0xA0,0x02,0xC1])) 
        elif text=='18': 
            ser.write(serial.to_bytes([0xA0,0x03,0xC1]))

    def SpO2_ADC_select(self, text):
        global event
        Clock.unschedule(event)
        event=Clock.schedule_once(self.timeout, 5)

        if text=='2048': 
            ser.write(serial.to_bytes([0xA0,0x00,0xC2])) 
        elif text=='4096': 
            ser.write(serial.to_bytes([0xA0,0x20,0xC2])) 
        elif text=='8192': 
            ser.write(serial.to_bytes([0xA0,0x40,0xC2])) 
        elif text=='16384': 
            ser.write(serial.to_bytes([0xA0,0x60,0xC2]))
  
    def Red_select(self, value):
        global event
        Clock.unschedule(event)
        event=Clock.schedule_once(self.timeout, 5)
        
        valueint=int(value*31/6200) 
        ser.write(serial.to_bytes([0xA0,valueint,0xC3]))
    
    def IR_select(self, value):
        global event
        Clock.unschedule(event)
        event=Clock.schedule_once(self.timeout, 5)
        
        valueint=int(value*31/6200) 
        ser.write(serial.to_bytes([0xA0,valueint,0xC4]))

    def sensitivity_select(self, value):
        global event
        Clock.unschedule(event)
        event=Clock.schedule_once(self.timeout, 5)
        
        valueint=int(value)
        ser.write(serial.to_bytes([0xA0,valueint,0xC5]))   

class WindowManager(ScreenManager):
    pass
kv=Builder.load_file("saturimeter.kv")
sm= WindowManager()

screens= [ConnectionWindow(name='connectionwindow'),MenuWindow(name='menuwindow'),SettingsWindow(name='settingswindow')]

for screen in screens:
    sm.add_widget(screen)

sm.current= 'connectionwindow'

class SwitchingscreenApp(App):
    def build(self):
        return sm

if __name__ == '__main__':
    SwitchingscreenApp().run()