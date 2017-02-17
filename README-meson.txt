Amlogic Framebuffer Mali Quake3
###############################

Build and install mali-framebuffer enabled libsdl2.

Build with :
make SDL_LIBS="-L/usr/local/lib -Wl,-rpath,/usr/local/lib -lSDL2" PLATFORM_HACK=gles BUILD_RENDERER_OPENGL2=0 USE_RENDERER_DLOPEN=0
