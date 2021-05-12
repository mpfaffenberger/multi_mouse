# -*- coding: utf-8 -*-
import glfw

try:
    import OpenGL as ogl
    try:
        import OpenGL.GL as gl  # this fails in <=2020 versions of Python on OS X 11.x
    except ImportError:
        print('Drat, patching for Big Sur')
        from ctypes import util
        orig_util_find_library = util.find_library
        def new_util_find_library( name ):
            res = orig_util_find_library( name )
            if res: return res
            return '/System/Library/Frameworks/'+name+'.framework/'+name
        util.find_library = new_util_find_library
except ImportError:
    pass

import socket
import imgui
import uuid
from imgui.integrations.glfw import GlfwRenderer


class MouseSocket:

    def __init__(self):
        self.is_connected = False

    def connect(self, ip_port):
        try:
            self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.id = str(uuid.uuid4())[0:3]
            ip, port = ip_port.split(":")
            self.s.connect((ip, int(port)))
            self.s.send("{}h".format(self.id).encode())
            self.is_connected = True

        except:
            self.is_connected = False
    
    def send_mouse_coords(self, x, y):
        if self.is_connected:
            self.s.send("{}m{}|{}".format(self.id, x, y).encode())

    def send_click(self, x, y):
        if self.is_connected:
            self.s.send("{}c{}|{}".format(self.id, x, y).encode())


def main():
    imgui.create_context()
    window = impl_glfw_init()
    impl = GlfwRenderer(window)
    ip_address = "localhost:3333"
    #imgui.set_window_font_scale(1.0) 
    comms = MouseSocket()
    click_guard = True
    
    while not glfw.window_should_close(window):
        glfw.poll_events()
        impl.process_inputs()
        imgui.new_frame()
        imgui.set_next_window_size(300, 120)
        imgui.set_next_window_position(10, 0)
        imgui.begin("", False, imgui.WINDOW_NO_RESIZE)
        changed, ip_address = imgui.input_text(
            label="IP:PORT", value=ip_address, buffer_length=30
        )
        clicked = imgui.button(
            label="CONNECT"
        )
        if clicked:
            comms.connect(ip_address)

        max_x, max_y = glfw.get_window_size(window)
        x, y = glfw.get_cursor_pos(window)

        norm_x = x / max_x
        norm_y = y / max_y

        state = glfw.get_mouse_button(window, glfw.MOUSE_BUTTON_LEFT);
        if state == glfw.PRESS:
            click_guard = False
        elif state == glfw.RELEASE and not click_guard:
            click_guard = True
            comms.send_click(norm_x, norm_y)

        comms.send_mouse_coords(norm_x, norm_y)

        imgui.text("Mouse Coords: {:.2f}, {:.2f}".format(norm_x, norm_y))    
        imgui.text("Connected: {}".format(comms.is_connected))
        imgui.end()

        gl.glClear(gl.GL_COLOR_BUFFER_BIT)
        imgui.render()
        impl.render(imgui.get_draw_data())
        glfw.swap_buffers(window)

    impl.shutdown()
    glfw.terminate()


def impl_glfw_init():
    width, height = 1280, 720
    window_name = "Multi Mouse Client"

    if not glfw.init():
        print("Could not initialize OpenGL context")
        exit(1)

    # OS X supports only forward-compatible core profiles from 3.2
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)

    glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, gl.GL_TRUE)
    glfw.window_hint(glfw.TRANSPARENT_FRAMEBUFFER, glfw.TRUE)

    # Create a windowed mode window and its OpenGL context
    window = glfw.create_window(
        int(width), int(height), window_name, None, None
    )
    glfw.make_context_current(window)

    if not window:
        glfw.terminate()
        print("Could not initialize Window")
        exit(1)

    return window


if __name__ == "__main__":
    main()