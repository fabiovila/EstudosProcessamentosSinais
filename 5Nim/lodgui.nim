import window
import sdl2
import gldriver

type 
    LodGui* = object
        windows: seq[Window]
        colors: Color
        driver: Driver

proc initLodGui*(w: cint = 200, h: cint = 200, x: cint = 0, y: cint = 0, title: string = ""): LodGui =
    result.windows = newSeq[Window](10)
    result.driver = InitGL(w,h,x,y,title)

# Window Manager cria janelas
proc CreateWindow (self: var LodGui, x: int32, y: int32, w: int32, h: int32, title: string): ref Window = 
    var win = initWindow(x,y,w,h,title)
    self.windows.add(win)

### Tem que trazer o SDL para o LodGUI
proc Render* (self: var LodGui) =
    var run: bool = true
    while run:
        while pollEvent(evt):

            if evt.kind == QuitEvent:
                run = false
                break
            if evt.kind == MouseMotion:
                button = sdl2.getMouseState(mousex.addr,mousey.addr)

            if evt.kind == WindowEvent:
                var windowEvent = cast[WindowEventPtr](addr(evt))
                if windowEvent.event == WindowEvent_Resized:
                    let newWidth = windowEvent.data1
                    let newHeight = windowEvent.data2
                    #reshape(newWidth, newHeight)
    self.driver.Swap()
    
