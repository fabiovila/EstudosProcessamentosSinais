import sdl2
import modules/gl 
import lodgui


let Lodgui = initLodGui()

#Lodgui.Render()




#[
const points = [
   0.0f,  0.5f,  0.0f,
   0.5f, -0.5f,  0.0f,
  -0.5f, -0.5f,  0.0f
]

var vbo : GLuint = 0
glGenBuffers(1, addr(vbo))
glBindBuffer(GL_ARRAY_BUFFER, vbo)

glBufferData(GL_ARRAY_BUFFER, int32(points.sizeof), addr(points), GL_STATIC_DRAW)

var vao : GLuint = 0
glGenVertexArrays(1, addr(vao))
glBindVertexArray(vao)
glEnableVertexAttribArray(0)
glBindBuffer(GL_ARRAY_BUFFER, vbo)
glVertexAttribPointer(GLuint(0), GLint(3), cGL_FLOAT, false, 0, cast[pointer](0))
]#
var 
  shader: GLuint
  mousex: cint
  mousey: cint
  button: uint8
#[
block:
  var vseq: seq[string] =  @[readFile("shaders/vertex.glsl")]
  var fseq: seq[string] =  @[readFile("shaders/fragment.glsl")]
  var fragment = allocCStringArray(fseq)
  var vertex = allocCStringArray(vseq)
  defer: dealloc(vertex)
  defer: dealloc(fragment)

  var  vs : GLuint = glCreateShader(GL_VERTEX_SHADER)
  glShaderSource(vs, GLsizei(1), vertex, nil)
  glCompileShader(vs);
  var  fs : GLuint = glCreateShader(GL_FRAGMENT_SHADER)
  glShaderSource(fs, GLsizei(1), fragment, nil)
  glCompileShader(fs)

  shader = glCreateProgram()
  glAttachShader(shader, fs)
  glAttachShader(shader, vs)
  glLinkProgram(shader)

]#


proc reshape(newWidth: cint, newHeight: cint) =
  glViewport(0, 0, newWidth, newHeight)   # Set the viewport to cover the new window
  glMatrixMode(GL_PROJECTION)             # To operate on the projection matrix
  glLoadIdentity()                        # Reset

#proc render() =
#  glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT)
#  glUseProgram(shader)
#  glBindVertexArray(vao);
#  glDrawArrays(GL_TRIANGLES, 0, 3)
#  window.glSwapWindow() # Swap the front and back frame buffers (double buffering)

# Frame rate limiter

let targetFramePeriod: uint32 = 20 # 20 milliseconds corresponds to 50 fps
var frameTime: uint32 = 0

proc limitFrameRate() =
  let now = getTicks()
  if frameTime > now:
    delay(frameTime - now) # Delay to maintain steady frame rate
  frameTime += targetFramePeriod

# Main loop

var
  evt = sdl2.defaultEvent
  runGame = true

#reshape(screenWidth, screenHeight) # Set up initial viewport and projection





while runGame:
  while pollEvent(evt):

    if evt.kind == QuitEvent:
      runGame = false
      break
    if evt.kind == MouseMotion:
      button = sdl2.getMouseState(mousex.addr,mousey.addr)

    if evt.kind == WindowEvent:
      var windowEvent = cast[WindowEventPtr](addr(evt))
      if windowEvent.event == WindowEvent_Resized:
        let newWidth = windowEvent.data1
        let newHeight = windowEvent.data2
        #reshape(newWidth, newHeight)

  #render()

  limitFrameRate()

#destroy window
glDeleteProgram(shader)