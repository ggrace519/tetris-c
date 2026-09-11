#include "app/postfx.hpp"

namespace tetris {

namespace {
// Bloom fragment shader (GLSL 330), adapted from raylib's example bloom.fs. The
// stock shader hardcodes the framebuffer size as a const; here `size` is a uniform
// so the glow is resolution-independent (set once in init()). Box-blur the texture
// and add it back to the source for a soft glow around bright pixels.
const char* kBloomFs = R"GLSL(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 size;       // framebuffer size in pixels (set from code)
out vec4 finalColor;

const float samples = 5.0;   // pixels per axis
const float quality = 2.5;   // glow spread

void main()
{
    vec4 sum = vec4(0.0);
    vec2 sizeFactor = vec2(1.0) / size * quality;
    vec4 source = texture(texture0, fragTexCoord);
    const int range = 2;     // (samples-1)/2
    for (int x = -range; x <= range; x++)
        for (int y = -range; y <= range; y++)
            sum += texture(texture0, fragTexCoord + vec2(float(x), float(y)) * sizeFactor);
    // Bias the glow toward already-bright pixels so dark areas don't wash out.
    vec4 glow = (sum / (samples * samples));
    finalColor = (source + glow * 0.55) * colDiffuse;
}
)GLSL";
}  // namespace

PostFx::~PostFx() {
    if (bloomLoaded_) UnloadShader(bloom_);
    if (ready_) UnloadRenderTexture(scene_);
}

void PostFx::init(int w, int h) {
    w_ = w;
    h_ = h;
    scene_ = LoadRenderTexture(w, h);
    if (scene_.id == 0) return;  // no GL / render target unavailable
    // Smooth scaling of the blit; POINT would keep it crisp but bloom wants bilinear.
    SetTextureFilter(scene_.texture, TEXTURE_FILTER_BILINEAR);

    bloom_ = LoadShaderFromMemory(nullptr, kBloomFs);
    if (bloom_.id != 0) {
        bloomLoaded_ = true;
        const int loc = GetShaderLocation(bloom_, "size");
        const float res[2] = {static_cast<float>(w), static_cast<float>(h)};
        SetShaderValue(bloom_, loc, res, SHADER_UNIFORM_VEC2);
    }
    ready_ = true;
}

void PostFx::begin() {
    if (!ready_) return;
    BeginTextureMode(scene_);
}

void PostFx::end(bool bloom) {
    if (!ready_) return;
    EndTextureMode();
    // Blit the scene texture to the screen. Render textures are Y-flipped, so the
    // source rect height is negative (raylib shaders_postprocessing.c convention).
    const Rectangle src{0.0f, 0.0f, static_cast<float>(w_), -static_cast<float>(h_)};
    const Rectangle dst{0.0f, 0.0f, static_cast<float>(GetScreenWidth()),
                        static_cast<float>(GetScreenHeight())};
    if (bloom && bloomLoaded_) {
        BeginShaderMode(bloom_);
        DrawTexturePro(scene_.texture, src, dst, Vector2{0, 0}, 0.0f, WHITE);
        EndShaderMode();
    } else {
        DrawTexturePro(scene_.texture, src, dst, Vector2{0, 0}, 0.0f, WHITE);
    }
}

}  // namespace tetris
