#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <fstream>

#include <nlohmann/json.hpp>
#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <cstdint>
#define IMGUI_ENABLE_DOCKING
#include <imgui.h>
#include "imgui_impl_sdl3.h"
#include "imgui_impl_bgfx.h"

bool runing_game = false;
using json = nlohmann::json;

std::string vs = "shaders/vs_voxel.bin";
std::string fs = "shaders/fs_voxel.bin";

struct Pos {
    float x = 0;
    float y = 0;
    float z = 0;
};
struct Rot {
    float x = 0;
    float y = 0;
    float z = 0;
};
struct Scale {
    float x = 1;
    float y = 1;
    float z = 1;
};
struct Transform {
    Pos position;
    Rot rotation;
    Scale scale;
};

class Node {
private:
    int cbsuid_max = 0;

public:
    std::string name = "new_node";
    std::vector<std::unique_ptr<Node>> children;
    Node* parent = nullptr;
    int suid = 0; // scene uid
    int guid = 0; // global uid
    bool is_inheret = false;
    std::string inhereting_scene = "";
    Transform transform;

    nlohmann::json to_dict() {
        nlohmann::json to_save;
        nlohmann::json transform_save;
        nlohmann::json children_save;

        to_save["name"] = name;
        to_save["suid"] = suid;
        to_save["guid"] = guid;
        to_save["is_inheret"] = is_inheret;
        to_save["inhereting_scene"] = inhereting_scene;

        transform_save["position"] = std::array<float, 3>{
            transform.position.x,
            transform.position.y, 
            transform.position.z
        };
        transform_save["rotation"] = std::array<float, 3>{
            transform.rotation.x,
            transform.rotation.y, 
            transform.rotation.z
        };
        transform_save["scale"] = std::array<float, 3>{
            transform.scale.x,
            transform.scale.y, 
            transform.scale.z
        };
        
        to_save["transform"] = transform_save;
        for (const auto& child : children) {
            if (child) {
                children_save[std::to_string(child->suid)] = child->to_dict();
            }
        }
        to_save["children"] = children_save;
        return to_save;
    }

    void save_tree(const std::string& filename) {
        nlohmann::json save = to_dict();
        std::ofstream file(filename);
        if (file.is_open()) {
            file << save.dump(4);
        }
    }

    void make_tree_inheret(const std::string& scene_file) {
        if (!runing_game) {
            is_inheret = true;
            inhereting_scene = scene_file;
            for (auto& child : children) {
                if (child) {
                    child->is_inheret = true;
                }
            }
            save_tree(name + ".json"); 
        }
    }

    void add_child(std::unique_ptr<Node> node) {
        if (!node) return;
        node->parent = this;
        node->suid = ++cbsuid_max;
        children.push_back(std::move(node));
    }

    void remove_child(Node* node) {
        children.erase(
            std::remove_if(children.begin(), children.end(),
                [node](const std::unique_ptr<Node>& c) { return c.get() == node; }),
            children.end()
        );
    }

    void remove_child_at_idx(int idx) {
        if (idx >= 0 && idx < static_cast<int>(children.size())) {
            children.erase(children.begin() + idx);
        }
    }

    void rename(const std::string& nn) {
        name = nn;
    }

    void reparent(Node* new_parent) {
        if (!new_parent) return;
        parent = new_parent;
        suid = ++new_parent->cbsuid_max;
    }
};

bgfx::ShaderHandle loadShader(const char* path){
    FILE* file = fopen(path, "rb");
    if (!file){
        std::cout << "failed to load file " << path << "\n";
        return BGFX_INVALID_HANDLE;
    }
    fseek(file, 0, SEEK_END);
    uint32_t size = static_cast<uint32_t>(ftell(file));
    fseek(file, 0, SEEK_SET);
    const bgfx::Memory* mem = bgfx::alloc(size);
    fread(mem->data, 1, size, file);
    fclose(file);
    mem->data[mem->size-1]='\0';
    return bgfx::createShader(mem);
}
bgfx::ShaderHandle loadShader2(const char* filename){
    std::ifstream file(filename, std::ios::binary|std::ios::ate);
    if (!file){
        std::cout << "failed load \n";
        return BGFX_INVALID_HANDLE; 
    }
    const std::streamsize size=file.tellg();

    if (size<=0){
        std::cout << "failed size \n";
        return BGFX_INVALID_HANDLE;
    };

    file.seekg(0,std::ios::beg);
    std::vector<char> data(static_cast<size_t>(size));

    if (!file.read(data.data(),size)){
        std::cout << "failed read \n";
        return BGFX_INVALID_HANDLE;
    };

    const bgfx::Memory* mem=bgfx::copy(data.data(),static_cast<uint32_t>(data.size()));

    bgfx::ShaderHandle sh=bgfx::createShader(mem);

    if (!bgfx::isValid(sh)){
        std::cout << "failed invalid \n";
        return BGFX_INVALID_HANDLE;
    };

    return sh;
}

int floorDiv(int a, int b) {
    if(a >= 0)
        return a / b;
    return (a - b + 1) / b;
}

int chunk_size = 16;
int chunk_height = 100;

struct ChunkPos {
    int x;
    int z;

    bool operator==(const ChunkPos& other) const {
        return x == other.x && z == other.z;
    }
};

int index(int x, int y, int z) {
    return x + z * chunk_size + y * chunk_size * chunk_size;
}

int worldToLocal(int global) {
    int local = global % chunk_size;
    if (local < 0)
        local += chunk_size;
    return local;
}

namespace std {
template<>
struct hash<ChunkPos> {
    size_t operator()(const ChunkPos& p) const {
        return hash<int>()(p.x) ^ (hash<int>()(p.z) << 1);
    }
};
}

struct Vertex {
    float x = 0;
    float y = 0;
    float z = 0;
    float u = 0;
    float v = 0;
};

struct Face {
    Vertex verts[4];
};

const Face faces[6] = {
    // Top
    {{ {0,1,0,0,0}, {0,1,1,1,0}, {1,1,0,0,1}, {1,1,1,1,1} }},
    // Bottom
    {{ {0,0,1,0,0}, {0,0,0,1,0}, {1,0,1,0,1}, {1,0,0,1,1} }},
    // Right
    {{ {1,0,1,0,0}, {1,0,0,1,0}, {1,1,1,0,1}, {1,1,0,1,1} }},
    // Left
    {{ {0,0,0,0,0}, {0,0,1,1,0}, {0,1,0,0,1}, {0,1,1,1,1} }},
    // Front
    {{ {0,0,1,0,0}, {1,0,1,1,0}, {0,1,1,0,1}, {1,1,1,1,1} }},
    // Back
    {{ {1,0,0,0,0}, {0,0,0,1,0}, {1,1,0,0,1}, {0,1,0,1,1} }}
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    void addFace(int face, int x, int y, int z){
        uint32_t start = vertices.size();
        for (int i = 0; i < 4; i++){
            Vertex v = faces[face].verts[i];
            v.x += x;
            v.y += y;
            v.z += z;
            vertices.push_back(v);
        }
        indices.push_back(start + 0);
        indices.push_back(start + 2);
        indices.push_back(start + 1);

        indices.push_back(start + 1);
        indices.push_back(start + 2);
        indices.push_back(start + 3);
    }
};

struct Block {
    uint8_t type = 0;
    bool isSolid() const {
        return type != 0;
    }
};

ChunkPos worldToChunk(int x, int z) {
    return {
        floorDiv(x, chunk_size),
        floorDiv(z, chunk_size)
    };
}

bool isAir(int globalX, int globalY, int globalZ);

class Chunk {
    ChunkPos position{0,0};
public:
    std::vector<Block> blocks;
    Mesh mesh;
    bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout layout;

    Chunk() {
        blocks.resize(chunk_size * chunk_height * chunk_size);
    }
    Chunk(ChunkPos pos) : position(pos) {
        blocks.resize(chunk_size * chunk_height * chunk_size);
    }

    Block& getBlock(int x, int y, int z) {
        return blocks[index(x, y, z)];
    }

    void init() {
        layout.begin().add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                      .end();
    }
    

    void make_mesh() {
        mesh.vertices.clear();
        mesh.indices.clear();
        for (int x = 0; x < chunk_size; x++) {
            for (int y = 0; y < chunk_height; y++) {
                for (int z = 0; z < chunk_size; z++) {
                    Block& block = blocks[index(x, y, z)];
                    if (!block.isSolid()) continue;

                    int global_blockX = position.x * chunk_size + x;
                    int global_blockZ = position.z * chunk_size + z;

                    if (isAir(global_blockX + 1, y, global_blockZ)) mesh.addFace(2, x, y, z);
                    if (isAir(global_blockX - 1, y, global_blockZ)) mesh.addFace(3, x, y, z);
                    if (isAir(global_blockX, y, global_blockZ + 1)) mesh.addFace(4, x, y, z);
                    if (isAir(global_blockX, y, global_blockZ - 1)) mesh.addFace(5, x, y, z);
                    if (isAir(global_blockX, y - 1, global_blockZ)) mesh.addFace(1, x, y, z);
                    if (isAir(global_blockX, y + 1, global_blockZ)) mesh.addFace(0, x, y, z);
                }
            }
        }
    }

    void make_layout() {
        if (bgfx::isValid(vbh)) bgfx::destroy(vbh);
        if (bgfx::isValid(ibh)) bgfx::destroy(ibh);
        if (mesh.vertices.empty() || mesh.indices.empty()) return;

        vbh = bgfx::createVertexBuffer(bgfx::copy(mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex)), layout);
        ibh = bgfx::createIndexBuffer(bgfx::copy(mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);
        std::cout << "layout\n";
    }
    
};

std::unordered_map<ChunkPos, Chunk> loaded_chunks;

bool isAir(int globalX, int globalY, int globalZ) {
    if (globalY < 0 || globalY >= chunk_height)
        return true;

    ChunkPos chunkPos = worldToChunk(globalX, globalZ);
    auto it = loaded_chunks.find(chunkPos);
    if (it == loaded_chunks.end())
        return true;

    Chunk& chunk = it->second;
    int localX = worldToLocal(globalX);
    int localZ = worldToLocal(globalZ);
    return !chunk.getBlock(localX, globalY, localZ).isSolid();
}

void generateChunks() {
    Chunk chunk({0, 0});
    for (int x = 0; x < chunk_size; x++) {
        for (int z = 0; z < chunk_size; z++) {
            for (int y = 0; y < 5; y++) {
                chunk.getBlock(x, y, z).type = 1;
            }
        }
    }
    loaded_chunks[{0, 0}] = std::move(chunk);
    loaded_chunks[{0, 0}].make_mesh();
}

int width = 1280;
int height = 720;
bool running = true;

int main() {
    generateChunks();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "failed sdl init\n";
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("VOXEL_GE - Editor", width, height, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cout << "failed window creation\n";
        return -1;
    }

    bgfx::PlatformData pd{};
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    pd.nwh = (void*)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    bgfx::setPlatformData(pd);

    bgfx::Init init;
    init.type = bgfx::RendererType::Count;
    init.resolution.height = height;
    init.resolution.width = width;
    init.resolution.reset = BGFX_RESET_VSYNC;
    init.platformData.nwh=pd.nwh;
    bgfx::init(init);

    bgfx::ShaderHandle vsh = loadShader("shaders/vs_voxel.bin");
    bgfx::ShaderHandle fsh = loadShader("shaders/fs_voxel.bin");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    #ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    #endif
    ImGui::StyleColorsDark();


    
    
    ImGui_ImplSDL3_InitForOther(window);

    ImGui_Implbgfx_Init(255);
    //ImGui::ShowDemoWindow();
    
    /*bgfx::ShaderHandle vsh = loadShader("shaders/vs_voxel.bin");
    bgfx::ShaderHandle fsh = loadShader("shaders/fs_voxel.bin");*/
    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        std::cout << "Failed to load shaders\n";
        return -1;
    }
    bgfx::ProgramHandle program = bgfx::createProgram(vsh, fsh, true);


    for (auto& [pos, chunk] : loaded_chunks) {
        chunk.init();
        chunk.make_layout();
    }

    Node root_node;
    root_node.name = "SceneRoot";

    auto voxel_node = std::make_unique<Node>();
    auto child1 = std::make_unique<Node>();
    voxel_node->name = "VoxelWorldNode";
    root_node.add_child(std::move(voxel_node));
    
    Node* selected_node = &root_node;
    uint16_t rt_width = 800;
    uint16_t rt_height = 600;

    bgfx::TextureHandle render_texture = bgfx::createTexture2D(
        rt_width, rt_height, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
    );
    bgfx::FrameBufferHandle framebuffer = bgfx::createFrameBuffer(1, &render_texture, true);
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                width = event.window.data1;
                height = event.window.data2;
                bgfx::reset(width, height, BGFX_RESET_VSYNC);
                bgfx::setViewRect(0, 0, 0, width, height);
            }
        }
        ImGui_Implbgfx_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        //ImGui::ShowDemoWindow();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        ImGui::Begin("Scene Hierarchy"/*, nullptr, ImGuiWindowFlags_NoMove*/);
        std::function<void(Node*)> draw_node_tree = [&](Node* node) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (node == selected_node) flags |= ImGuiTreeNodeFlags_Selected;
            if (node->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

            bool opened = ImGui::TreeNodeEx((void*)node, flags, "%s", node->name.c_str());
            if (ImGui::IsItemClicked()) {
                selected_node = node;
            }
            if (opened) {
                for (auto& child : node->children) {
                    draw_node_tree(child.get());
                }
                ImGui::TreePop();
            }
        };
        draw_node_tree(&root_node);
        ImGui::End();

        ImGui::Begin("Inspector"/*, nullptr,ImGuiWindowFlags_NoMove*/);
        if (selected_node) {
            char name_buf[256];
            memset(name_buf, 0, sizeof(name_buf));
            strncpy(name_buf, selected_node->name.c_str(), sizeof(name_buf) - 1);
            if (ImGui::InputText("Node Name", name_buf, sizeof(name_buf))) {
                selected_node->name = name_buf;
            }

            ImGui::Separator();
            ImGui::Text("Transform Properties");
            ImGui::DragFloat3("Position", &selected_node->transform.position.x, 0.1f);
            ImGui::DragFloat3("Rotation", &selected_node->transform.rotation.x, 0.5f);
            ImGui::DragFloat3("Scale", &selected_node->transform.scale.x, 0.01f, 0.01f, 100.0f);

            ImGui::Separator();
            ImGui::Checkbox("Run Game Mode", &runing_game);
            if (ImGui::Button("Save Scene / Inherit")) {
                selected_node->make_tree_inheret("scene_config");
            }
        } else {
            ImGui::Text("Select a node to inspect.");
        }
        
        ImGui::End();

        ImGui::Begin("Engine Stats"/*,nullptr, ImGuiWindowFlags_NoMove*/);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Loaded Chunks: %zu", loaded_chunks.size());
        for (auto& [pos, chunk] : loaded_chunks) {
            ImGui::Text("Chunk [%d, %d] Vertices: %zu", pos.x, pos.z, chunk.mesh.vertices.size());
        }
        ImGui::End();

        ImGui::Begin("Viewport"/*, nullptr, ImGuiWindowFlags_NoMove*/);
        ImVec2 panel_size = ImGui::GetContentRegionAvail();

        if (panel_size.x > 0 && panel_size.y > 0) {
            uint16_t new_w = static_cast<uint16_t>(panel_size.x);
            uint16_t new_h = static_cast<uint16_t>(panel_size.y);


            if (new_w != rt_width || new_h != rt_height) {
                rt_width = new_w;
                rt_height = new_h;

                bgfx::destroy(framebuffer);
                bgfx::destroy(render_texture);

                render_texture = bgfx::createTexture2D(
                    rt_width, rt_height, false, 1, bgfx::TextureFormat::RGBA8,
                    BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
                );
                framebuffer = bgfx::createFrameBuffer(1, &render_texture, true);
            }
        }
        ImGui::Image((ImTextureID)(uintptr_t)render_texture.idx, panel_size);
        ImGui::End();


        bgfx::setViewFrameBuffer(0, framebuffer);
        bgfx::setViewRect(0, 0, 0, rt_width, rt_height);

        float view[16];
        bx::mtxLookAt(view, bx::Vec3{20.0f, 20.0f, 20.0f}, bx::Vec3{0.0f, 0.0f, 0.0f});

        float proj[16];
        bx::mtxProj(proj, 60.0f, float(rt_width) / float(rt_height), 0.1f, 1000.0f, bgfx::getCaps()->homogeneousDepth);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff);
        bgfx::setViewTransform(0, view, proj);

        for (auto& [pos, chunk] : loaded_chunks) {
            if (!bgfx::isValid(chunk.vbh)) continue;
            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW);
            bgfx::setVertexBuffer(0, chunk.vbh);
            bgfx::setIndexBuffer(chunk.ibh);
            bgfx::submit(0, program);
        }

        bgfx::touch(0);

        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        bgfx::frame();

    }
    ImGui_Implbgfx_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
