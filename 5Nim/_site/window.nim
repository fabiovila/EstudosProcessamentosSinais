import  lodgui

type
    Color* = object
        r,g,b,a : float32

type 
    Window* = object
        x,y,w,h: int32
        title: string
        handler: int
        grabbing, moving, resizing: bool
        backgroundColor: Color
        show, active: bool

proc initWindow* (x: int32 = 10, y: int32 = 10, w: int32 = 150, h: int32 = 150, title: string = ""): Window = 
    result.x = x
    result.y = y
    result.w = w
    result.h = h
    result.title = title

