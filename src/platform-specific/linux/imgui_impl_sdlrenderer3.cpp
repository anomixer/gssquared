// imgui_impl_sdlrenderer3.cpp – minimal SDL_Renderer backend for Dear ImGui 1.90.1
// Written against SDL3 3.3.0 API (SDL_FColor, new SDL_RenderGeometryRaw, etc.)
// Implements the same public interface as the upstream imgui_impl_sdlrenderer3
// so that imgui_impl_sdlrenderer3.h (system header) is satisfied.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "backends/imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>
#include <cstring>
#include <vector>

// ── Backend data ──────────────────────────────────────────────────────────────

struct ImGui_ImplSDLRenderer3_Data {
    SDL_Renderer* SDLRenderer;
    SDL_Texture*  FontTexture;

    ImGui_ImplSDLRenderer3_Data() { memset((void*)this, 0, sizeof(*this)); }
};

static ImGui_ImplSDLRenderer3_Data* ImGui_ImplSDLRenderer3_GetBackendData()
{
    return ImGui::GetCurrentContext()
        ? (ImGui_ImplSDLRenderer3_Data*)ImGui::GetIO().BackendRendererUserData
        : nullptr;
}

// ── Font texture ──────────────────────────────────────────────────────────────

static void ImGui_ImplSDLRenderer3_SetupRenderState()
{
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();
    SDL_SetRenderViewport(bd->SDLRenderer, nullptr);
}

bool ImGui_ImplSDLRenderer3_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    SDL_Texture* texture = SDL_CreateTexture(
        bd->SDLRenderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STATIC,
        width, height);
    if (!texture) return false;

    SDL_UpdateTexture(texture, nullptr, pixels, width * 4);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);

    bd->FontTexture = texture;
    io.Fonts->SetTexID((ImTextureID)(intptr_t)texture);
    return true;
}

void ImGui_ImplSDLRenderer3_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();
    if (bd->FontTexture) {
        io.Fonts->SetTexID(0);
        SDL_DestroyTexture(bd->FontTexture);
        bd->FontTexture = nullptr;
    }
}

// ── Public API ────────────────────────────────────────────────────────────────

bool ImGui_ImplSDLRenderer3_Init(SDL_Renderer* renderer)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized!");

    ImGui_ImplSDLRenderer3_Data* bd = IM_NEW(ImGui_ImplSDLRenderer3_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName     = "imgui_impl_sdlrenderer3";
    io.BackendFlags           |= ImGuiBackendFlags_RendererHasVtxOffset;

    bd->SDLRenderer = renderer;
    return true;
}

void ImGui_ImplSDLRenderer3_Shutdown()
{
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDLRenderer3_DestroyFontsTexture();

    io.BackendRendererName     = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags           &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    IM_DELETE(bd);
}

void ImGui_ImplSDLRenderer3_NewFrame()
{
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplSDLRenderer3_Init()?");

    if (!bd->FontTexture)
        ImGui_ImplSDLRenderer3_CreateFontsTexture();
}

void ImGui_ImplSDLRenderer3_RenderDrawData(ImDrawData* draw_data)
{
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();

    // Avoid rendering when minimised
    int fb_width  = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) return;

    // Save render state
    SDL_Rect   old_viewport = {};
    SDL_Rect   old_clip     = {};
    bool       old_clip_en  = SDL_RenderClipEnabled(bd->SDLRenderer);
    SDL_GetRenderViewport(bd->SDLRenderer, &old_viewport);
    SDL_GetRenderClipRect(bd->SDLRenderer, &old_clip);

    ImGui_ImplSDLRenderer3_SetupRenderState();

    ImVec2 clip_off   = draw_data->DisplayPos;
    ImVec2 clip_scale = draw_data->FramebufferScale;

    // Per-vertex SDL_FColor scratch buffer (SDL3 requires float colors)
    static std::vector<SDL_FColor> fcolor_buf;

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buf  = cmd_list->VtxBuffer.Data;
        const ImDrawIdx*  idx_buf  = cmd_list->IdxBuffer.Data;
        int vtx_count = cmd_list->VtxBuffer.Size;

        // Convert packed ImU32 RGBA → SDL_FColor for every vertex
        fcolor_buf.resize(vtx_count);
        for (int i = 0; i < vtx_count; i++) {
            ImU32 c = vtx_buf[i].col;
            fcolor_buf[i].r = ((c >>  0) & 0xFF) / 255.0f;
            fcolor_buf[i].g = ((c >>  8) & 0xFF) / 255.0f;
            fcolor_buf[i].b = ((c >> 16) & 0xFF) / 255.0f;
            fcolor_buf[i].a = ((c >> 24) & 0xFF) / 255.0f;
        }

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            if (pcmd->UserCallback) {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplSDLRenderer3_SetupRenderState();
                else
                    pcmd->UserCallback(cmd_list, pcmd);
                continue;
            }

            // Scissor / clip rect
            ImVec2 clip_min(
                (pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
            ImVec2 clip_max(
                (pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
            if (clip_min.x < 0.0f) clip_min.x = 0.0f;
            if (clip_min.y < 0.0f) clip_min.y = 0.0f;
            if (clip_max.x > (float)fb_width)  clip_max.x = (float)fb_width;
            if (clip_max.y > (float)fb_height) clip_max.y = (float)fb_height;
            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) continue;

            SDL_Rect clip_r = {
                (int)clip_min.x, (int)clip_min.y,
                (int)(clip_max.x - clip_min.x),
                (int)(clip_max.y - clip_min.y)
            };
            SDL_SetRenderClipRect(bd->SDLRenderer, &clip_r);

            SDL_Texture* tex = (SDL_Texture*)(intptr_t)pcmd->GetTexID();

            const float* xy = (const float*)(const void*)(
                (const char*)(vtx_buf + pcmd->VtxOffset) + offsetof(ImDrawVert, pos));
            const float* uv = (const float*)(const void*)(
                (const char*)(vtx_buf + pcmd->VtxOffset) + offsetof(ImDrawVert, uv));
            const SDL_FColor* col = fcolor_buf.data() + pcmd->VtxOffset;

            SDL_RenderGeometryRaw(
                bd->SDLRenderer, tex,
                xy,  (int)sizeof(ImDrawVert),
                col, (int)sizeof(SDL_FColor),
                uv,  (int)sizeof(ImDrawVert),
                cmd_list->VtxBuffer.Size - (int)pcmd->VtxOffset,
                idx_buf + pcmd->IdxOffset,
                (int)pcmd->ElemCount,
                (int)sizeof(ImDrawIdx));
        }
    }

    // Restore render state
    SDL_SetRenderViewport(bd->SDLRenderer, &old_viewport);
    SDL_SetRenderClipRect(bd->SDLRenderer, old_clip_en ? &old_clip : nullptr);
}

bool ImGui_ImplSDLRenderer3_CreateDeviceObjects()
{
    return ImGui_ImplSDLRenderer3_CreateFontsTexture();
}

void ImGui_ImplSDLRenderer3_DestroyDeviceObjects()
{
    ImGui_ImplSDLRenderer3_DestroyFontsTexture();
}

#endif // #ifndef IMGUI_DISABLE
