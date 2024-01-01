#include "../include/shaders.h"
#include "../otags/obj.h"
#include "../otags/list.h"
#include "../otags/buffer.h"
#include <glob.h>
#include <stdio.h>



Object LoadShaders (Object Path) {
  tassert(Path,STRING);

  glob_t gl;
  int r = glob(cstr(Path),0,NULL,&gl);
  Object ShaderList = lNew(NULL);
  if (r == 0) {
    int i = 0, size;
    while (gl.gl_pathv[i])
    {
          printf("Loading Shader %s \n", gl.gl_pathv[i]);
          Object Path = o(gl.gl_pathv[i]); 
          Object B = bFromFile(Path);
          i++;
    }
  }

}