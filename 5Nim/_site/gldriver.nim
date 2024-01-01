import sdl2
import modules/gl 
import box
import std/[os, strutils, paths]


type 
    Driver* = object
        SDLwindow: WindowPtr
        GLcontext: GlContextPtr
        Shaders:    seq[GLuint]
        Program:    GLuint 
        windowmetrics: Box


proc LoadShaders(self: var Driver, path: string = "shaders/"): bool = 
    var 
        success: GLint
        log = newString(1024)
        compile: bool = false
        shader: GLuint

    for file in walkDir(path):
        if file.kind == pcFile or file.kind == pcLinktoFile:
            let ext = splitFile(file.path)[2]
            compile = false
            if ext == ".frag":
                shader = glCreateShader(GL_FRAGMENT_SHADER)
                compile = true
            elif ext == ".vert":
                shader = glCreateShader(GL_VERTEX_SHADER)
                compile = true

            if compile:
                var shadercstring = allocCStringArray([readFile(file.path)])
                defer: dealloc(shadercstring)
                
                glShaderSource(shader, GLsizei(1), shadercstring, nil)
                glCompileShader(shader)
                glGetShaderiv(shader, GL_COMPILE_STATUS, success.addr)
                if success == GL_FALSE:
                    glGetShaderInfoLog(shader, GLsizei(512), nil, log[0].addr);
                    echo "\x1b[31;1;4mError in " , file.path, ": ", log , "\x1b[0m"
                else:
                    self.Shaders.add(shader)

    self.Program = glCreateProgram()
    echo int(glGetError())
    if self.Program != 0:
        for i,s in self.Shaders:
            echo i , " ", s, " " ,self.Program
            glAttachShader(self.Program, s)
            echo int(glGetError())
        glLinkProgram(self.Program)
        echo int(glGetError())
        glGetProgramiv(self.Program, GL_LINK_STATUS, addr(success))
        if success == GL_FALSE:
            glGetProgramInfoLog(self.Program, 512, nil, log[0].addr)
            echo "\x1b[31;1;4mError linking program: ", log , "\x1b[0m"



    return true

proc InitGL*(w: cint = 150,h: cint = 150, x: cint = 10,y: cint = 10, title: cstring = ""): Driver = 
    result.windowmetrics.x = x
    result.windowmetrics.y = y
    result.windowmetrics.h = h
    result.windowmetrics.w = w
    result.Shaders = newSeq[Gluint](0)

    let sdlr: SDL_Return = sdl2.init(INIT_EVERYTHING)
    result.SDLwindow = createWindow(title, result.windowmetrics.x, result.windowmetrics.y, result.windowmetrics.w, result.windowmetrics.h, SDL_WINDOW_OPENGL or SDL_WINDOW_RESIZABLE)
    result.GLcontext = result.SDLwindow.glCreateContext()

    let gladr = gladLoadGL(sdl2.glGetProcAddress)
    if not gladr:
        echo "Falha ao carregar extensões com Glad"

    let renderer: ptr GLubyte = glGetString(GL_RENDERER)
    let version: ptr GLubyte = glGetString(GL_VERSION)
    echo "Renderer: ", cast[cstring](renderer)
    echo "OpenGL version supported: ", cast[cstring](version)

    glEnable(GL_DEPTH_TEST)
    glDepthFunc(GL_LESS)
    discard glSetAttribute(SDL_GL_DOUBLEBUFFER, 1)
    discard glSetAttribute(SDL_GL_DEPTH_SIZE, 24)
    discard result.LoadShaders()

proc Swap* (self: Driver) =
    glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT)
    self.SDLwindow.glSwapWindow()