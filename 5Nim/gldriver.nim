import sdl2
import modules/gl 
import box
import std/[os, strutils, paths]
import bitops

type Shaders* = object
        Id: GLuint
        Path: string

type 
    Driver* = object
        SDLwindow: WindowPtr
        GLcontext: GlContextPtr
        Shaders:    seq[Shaders]
        Program:    GLuint 
        windowmetrics: Box
        vao, vbo: GLuint


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
            #elif ext == ".geom":
                #shader = glCreateShader(GL_GEOMETRY_SHADER)
                #compile = true

            if compile and shader != 0:
                var shadercstring = allocCStringArray([readFile(file.path)])
                defer: dealloc(shadercstring)
                
                glShaderSource(shader, GLsizei(1), shadercstring, nil)
                glCompileShader(shader)
                glGetShaderiv(shader, GL_COMPILE_STATUS, success.addr)
                if success == GL_FALSE:
                    glGetShaderInfoLog(shader, GLsizei(512), nil, log[0].addr);
                    echo "\x1b[31;1;4mError in " , file.path, ": ", log , "\x1b[0m"
                else:
                    echo "Found shader: ", file.path
                    self.Shaders.add(Shaders(Id: shader, Path: file.path ))
            else:
                echo "\x1b[31;1;4mError in " , file.path, "\x1b[0m"

    self.Program = glCreateProgram()

    if self.Program != 0:
        for i,s in self.Shaders:            
            glAttachShader(self.Program, s.Id)
            if glGetError() != GL_INVALID_VALUE or glGetError() != GL_INVALID_OPERATION :
                echo "Shader ",s.Path," has been attached without errors."
        glLinkProgram(self.Program)

        glGetProgramiv(self.Program, GL_LINK_STATUS, addr(success))
        if success == GL_FALSE:
            glGetProgramInfoLog(self.Program, 512, nil, log[0].addr)
            echo "\x1b[31;1;4mError linking program: ", log , "\x1b[0m"
    var posAttrib: GLint = glGetAttribLocation(self.Program, "position")
    glEnableVertexAttribArray(GLuint(posAttrib))
    glVertexAttribPointer(GLuint(posAttrib), GLint(2), cGL_FLOAT, false, GLsizei(0), cast[pointer](0));

    glUseProgram(self.Program)

    

    return true


proc Box(self: var Driver) =
    const points = [
    0.0f,  0.5f,  0.0f,
    0.5f, -0.5f,  0.0f,
    -0.5f, -0.5f,  0.0f
    ]

    self.vbo = 0
    glGenBuffers(1, addr(self.vbo))
    glBindBuffer(GL_ARRAY_BUFFER, self.vbo)

    glBufferData(GL_ARRAY_BUFFER, int32(points.sizeof), addr(points), GL_STATIC_DRAW)

    self.vao = 0
    glGenVertexArrays(1, addr(self.vao))
    glBindVertexArray(self.vao)
    glEnableVertexAttribArray(0)
    glBindBuffer(GL_ARRAY_BUFFER, self.vbo)
    glVertexAttribPointer(GLuint(0), GLint(3), cGL_FLOAT, false, 0, cast[pointer](0))

proc InitGL*(w: cint = 200,h: cint = 200, x: cint = 10,y: cint = 10, title: cstring = ""): Driver = 
    result.windowmetrics.x = x
    result.windowmetrics.y = y
    result.windowmetrics.h = h
    result.windowmetrics.w = w
    result.Shaders = newSeq[Shaders](0)

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

proc WindowResized*(self: var Driver, wm: Box) =
    glViewport(0, 0, wm.w, wm.h)   
    glMatrixMode(GL_PROJECTION)             
    glLoadIdentity()

proc Swap* (self: var Driver) =
    glClearColor(0.0, 0.0, 0.0, 1.0)
    glClear(GLbitfield(bitor(GL_COLOR_BUFFER_BIT.ord, GL_DEPTH_BUFFER_BIT.ord)))
    glUseProgram(self.Program)
    glBindVertexArray(self.vao);
    glDrawArrays(GL_TRIANGLES, 0, 3)

   
    self.SDLwindow.glSwapWindow()