import window
import gldriver

type 
    LodGui* = object
        windows: seq[Window]
        colors: Color
        driver: Driver

proc initLodGui*(): LodGui =
    result.windows = newSeq[Window](10)
    result.driver = InitGL()

# Window Manager cria janelas
proc CreateWindow (self: var LodGui, x: int32, y: int32, w: int32, h: int32, title: string): ref Window = 
    var win = initWindow(x,y,w,h,title)
    self.windows.add(win)


proc Render* (self: var LodGui) =
    self.driver.Swap()
    
