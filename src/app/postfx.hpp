// Post-processing pipeline: renders the whole scene into a RenderTexture2D and
// blits it to the screen, optionally through a bloom (glow) shader. App-layer.
// (INNOVATIONS.md #5 render target + #7 bloom.) Asset-free — the shader is an
// embedded GLSL 330 string, loaded via LoadShaderFromMemory.
#ifndef TETRIS_APP_POSTFX_HPP
#define TETRIS_APP_POSTFX_HPP

#include "raylib.h"

namespace tetris {

class PostFx {
public:
    PostFx() = default;
    ~PostFx();
    PostFx(const PostFx&) = delete;
    PostFx& operator=(const PostFx&) = delete;

    // Create the render target (w x h) and load the bloom shader. Call once after
    // the window/GL context exists. Safe to call when GL is unavailable (no-op).
    void init(int w, int h);

    // Begin drawing the scene into the render target.
    void begin();
    // Finish: blit the render target to the screen. If bloom is true and the shader
    // loaded, the blit goes through the bloom shader (glow).
    void end(bool bloom);

    bool ready() const { return ready_; }

private:
    RenderTexture2D scene_{};
    Shader bloom_{};
    bool bloomLoaded_ = false;
    bool ready_ = false;
    int w_ = 0, h_ = 0;
};

}  // namespace tetris

#endif  // TETRIS_APP_POSTFX_HPP
