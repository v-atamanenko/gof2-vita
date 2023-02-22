#include <math.h>

so_hook setGameOrientation_hook;
void setGameOrientation(void * this, int mode) {
    SO_CONTINUE(void *, setGameOrientation_hook, this, 2);
}

int getDisplayHeight(void * this) {
    return 544;
}

int getDisplayWidth(void * this) {
    return 960;
}

so_hook NewsTicker_Constructor_hook;
void NewsTicker_Constructor(void* this, int posX, int posY, int width, int param4, int param5) {
    // In some parts of the game, it still thinks of portrait orientation as primary,
    // thus we need to invert to multiply horizontal coordinates by the aspect ratio here
    int posX_real = (int) floorf((float)posX * (960.f / 544.f));
    int width_real = (int) floorf((float)width * (960.f / 544.f));
    SO_CONTINUE(void *, NewsTicker_Constructor_hook, this, posX_real, posY, width_real, param4, param5 );
}

void patch__screen_fix() {
    hook_addr(so_symbol(&so_mod, "_ZN11AbyssEngine6Engine16GetDisplayHeightEv"), (uintptr_t)getDisplayHeight);
    hook_addr(so_symbol(&so_mod, "_ZN11AbyssEngine6Engine15GetDisplayWidthEv"), (uintptr_t)getDisplayWidth);

    setGameOrientation_hook = hook_addr(so_symbol(&so_mod, "_ZN11AbyssEngine11PaintCanvas18SetGameOrientationENS_13LandscapeModeE"), (uintptr_t)setGameOrientation);
    NewsTicker_Constructor_hook = hook_addr(so_symbol(&so_mod, "_ZN10NewsTickerC1Eiiiii"), (uintptr_t) NewsTicker_Constructor);
}
