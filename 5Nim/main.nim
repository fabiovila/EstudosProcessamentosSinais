import sdl2
import modules/gl 
import lodgui


var Lodgui = initLodGui(300, 300)


var 
  shader: GLuint
  mousex: cint
  mousey: cint
  button: uint8


proc reshape(newWidth: cint, newHeight: cint) =
  glViewport(0, 0, newWidth, newHeight)   # Set the viewport to cover the new window
  glMatrixMode(GL_PROJECTION)             # To operate on the projection matrix
  glLoadIdentity()                        # Reset


let targetFramePeriod: uint32 = 20 # 20 milliseconds corresponds to 50 fps
var frameTime: uint32 = 0

proc limitFrameRate() =
  let now = getTicks()
  if frameTime > now:
    delay(frameTime - now) # Delay to maintain steady frame rate
  frameTime += targetFramePeriod


var
  evt = sdl2.defaultEvent
  runGame = true

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

  Lodgui.Render()

  limitFrameRate()


glDeleteProgram(shader)