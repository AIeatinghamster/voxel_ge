#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
//#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <cstdint>
#include <imgui.h>
#include <unordered_map>
#include <fstream>

std::string vs="shaders/vs_voxel.bin";
std::string fs="shaders/fs_voxel.bin";
/*bgfx::ShaderHandle loadShader(const char* path){
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if (!file){
        std::cout<<"failed to load file " << path;
        return BGFX_INVALID_HANDLE;
    };
    std::cout << "1\n";
    std::streamsize size=file.tellg();
    std::cout << "2\n";
    file.seekg(0);
    std::cout << "3\n";
    const bgfx::Memory* mem=bgfx::alloc((uint32_t)size);
    std::cout << "4\n";
    file.read((char*)mem->data,size);
    std::cout << "5\n";
    return bgfx::createShader(mem);
}*/
bgfx::ShaderHandle loadShader(const char* path){
    FILE* file=fopen(path,"rb");
    if (!file){
        std::cout<<"failed to load file " << path;
        return BGFX_INVALID_HANDLE;
    };
    std::cout << "1\n";
    fseek(file,0,SEEK_END);
    std::cout << "2\n";
    uint32_t size = static_cast<uint32_t>(ftell(file));
    std::cout << "3 " << size << "\n";
    fseek(file,0,SEEK_SET);
    std::cout << "4\n";
    const bgfx::Memory* mem=bgfx::alloc(size);
    std::cout << "5\n";
    fread(mem->data,1,size,file);
    std::cout << "6\n";
    fclose(file);
    std::cout << "7\n";
    mem->data[mem->size-1]='\0';
    std::cout << "8\n";
    return bgfx::createShader(mem);
}
int floorDiv(int a, int b)
{
    if(a >= 0)
        return a / b;

    return (a - b + 1) / b;
}
class Chunk;
int chunk_size=16;
int chunk_height=100;
struct ChunkPos
{
    int x;
    int z;

    bool operator==(const ChunkPos& other) const
    {
        return x == other.x && z == other.z;
    }
};

int index(int x, int y, int z)
{
    return x + z * chunk_size + y * chunk_size * chunk_size;
}
int worldToLocal(int global)
{
    int local = global % chunk_size;

    if (local < 0)
        local += chunk_size;

    return local;
}
namespace std
{
template<>
struct hash<ChunkPos>
{
    size_t operator()(const ChunkPos& p) const
    {
        return hash<int>()(p.x) ^ (hash<int>()(p.z) << 1);
    }
};
}
class Node{
    int uid=0;
    std::string name = "NewNode";
    float position_x=0;
    float position_y=0;
    float position_z=0;
    float rotation_x=0;
    float rotation_y=0;
    float rotation_z=0;
    float scale=1;
    std::vector<Node> children;
};

class Scene{
    Node root_node;
    bool active=false;
};
struct Vertex{
    float x = 0;
    float y = 0;
    float z = 0;
    float u = 0;
    float v = 0;
};
struct Face{
    Vertex verts[4];
};
const Face faces[6] =
{
    // Top
    {{
        {0,1,0,0,0},
        {0,1,1,1,0},
        {1,1,0,0,1},
        {1,1,1,1,1}
    }},

    // Bottom
    {{
        {0,0,1,0,0},
        {0,0,0,1,0},
        {1,0,1,0,1},
        {1,0,0,1,1}
    }},
    /*{{
        {0,0,1,0,0},
        {1,0,1,1,0},
        {0,0,0,0,1},
        {1,0,0,1,1}
    }},*/
    // Right
    {{
        {1,0,1,0,0},
        {1,0,0,1,0},
        {1,1,1,0,1},
        {1,1,0,1,1}
    }},

    // Left
    {{
        {0,0,0,0,0},
        {0,0,1,1,0},
        {0,1,0,0,1},
        {0,1,1,1,1}
    }},

    // Front
    {{
        {0,0,1,0,0},
        {1,0,1,1,0},
        {0,1,1,0,1},
        {1,1,1,1,1}
    }},

    // Back
    {{
        {1,0,0,0,0},
        {0,0,0,1,0},
        {1,1,0,0,1},
        {0,1,0,1,1}
    }}
};


class Mesh{
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

            /*indices.push_back(start + 0);
            indices.push_back(start + 1);
            indices.push_back(start + 2);

            indices.push_back(start + 2);
            indices.push_back(start + 1);
            indices.push_back(start + 3);*/
        }
};
struct Block
{
    uint8_t type = 0;

    bool isSolid() const
    {
        return type != 0;
    }
};

ChunkPos worldToChunk(int x,int z)
{
    return {
        floorDiv(x, chunk_size),
        floorDiv(z, chunk_size)
    };
}

class Structure{
    std::vector<Block> blocks;
};



bool isAir(int globalX, int globalY, int globalZ);

class Chunk{
    ChunkPos position{0,0};
    public:
        std::vector<Block> blocks;
        Mesh mesh;
        bool dirty = false;
        bgfx::VertexBufferHandle vbh=BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle ibh=BGFX_INVALID_HANDLE;
        Chunk(){
            blocks.resize(chunk_size * chunk_height * chunk_size);
        }
        Chunk(ChunkPos pos) : position(pos){
            blocks.resize(chunk_size * chunk_height * chunk_size);
        }

        Block& getBlock(int x,int y,int z)
        {
            return blocks[index(x,y,z)];
        }
        void init(){
            layout.begin().add(bgfx::Attrib::Position,3, bgfx::AttribType::Float).add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float).end();
        }
        bgfx::VertexLayout layout;
    void make_mesh(){
        std::cout<< "meshing\n";
        mesh.vertices.clear();
        mesh.indices.clear();
        for (int x=0;x<chunk_size; x++){
            for (int y=0;y<chunk_height; y++){
                for (int z=0; z<chunk_size; z++){
                    Block& block=blocks[index(x,y,z)];

                    if (!block.isSolid()){
                        continue;
                    }
                    int global_blockX = position.x * chunk_size + x;
                    int global_blockZ = position.z * chunk_size + z;
                    //x+
                    if (isAir(global_blockX+1,y,global_blockZ)){

                        mesh.addFace(2,x,y,z);
                    }
                    //x-
                    if (isAir(global_blockX-1,y,global_blockZ)){

                        mesh.addFace(3,x,y,z);
                    }
                    //z+
                    if (isAir(global_blockX,y,global_blockZ+1)){

                        mesh.addFace(4,x,y,z);
                    }
                    //z-
                    if (isAir(global_blockX,y,global_blockZ-1)){
                        mesh.addFace(5,x,y,z);
                    }
                    //y+
                    if ((isAir(global_blockX,y-1,global_blockZ))){

                        mesh.addFace(1,x,y,z);
                    }
                    //y-
                    if (isAir(global_blockX,y+1,global_blockZ)){
                        mesh.addFace(0,x,y,z);
                    }
                }
            }
            /*mesh.vertices.clear();
            mesh.indices.clear();

            mesh.addFace(0, 0, 0, 0);
            mesh.addFace(1, 0, 0, 0);
            mesh.addFace(2, 0, 0, 0);
            mesh.addFace(3, 0, 0, 0);
            mesh.addFace(4, 0, 0, 0);
            mesh.addFace(5, 0, 0, 0);*/
        }
        std::cout<< "meshed\n";

    }
    void make_layout(){
        if (bgfx::isValid(vbh)){
            bgfx::destroy(vbh);
        }
        if (bgfx::isValid(ibh)){
            bgfx::destroy(ibh);
        }
        vbh = bgfx::createVertexBuffer(bgfx::copy(mesh.vertices.data(), mesh.vertices.size()*sizeof(Vertex)),layout);
        ibh = bgfx::createIndexBuffer(bgfx::copy(mesh.indices.data(),mesh.indices.size()*sizeof(uint32_t)), BGFX_BUFFER_INDEX32);
        std::cout << "Vertices: " << mesh.vertices.size() << '\n';
        std::cout << "Indices: " << mesh.indices.size() << '\n';
    }
};
std::unordered_map<ChunkPos, Chunk> loaded_chunks;

bool isAir(int globalX, int globalY, int globalZ){
    if(globalY < 0 || globalY >= chunk_height)
        return true;

    ChunkPos chunkPos = worldToChunk(globalX, globalZ);

    auto it = loaded_chunks.find(chunkPos);

    if(it == loaded_chunks.end())
        return true;

    Chunk& chunk = it->second;

    int localX = worldToLocal(globalX);
    int localZ = worldToLocal(globalZ);

    return !chunk.getBlock(localX, globalY, localZ).isSolid();
}
Mesh world_mesh;
void generateChunks(){
    //tmp ignor rand_noise
    Chunk chunk({0,0});

    for(int x=0;x<chunk_size;x++)
    {
        for(int z=0;z<chunk_size;z++)
        {
            for(int y=0;y<5;y++)
            {
                chunk.getBlock(x,y,z).type = 1;
            }
        }
    }

    loaded_chunks[{0,0}] = std::move(chunk);

    loaded_chunks[{0,0}].make_mesh();
}
bool running=true;
int width=1080;
int height=900;
int main(){
    generateChunks();
    SDL_Init(SDL_INIT_VIDEO);
    if (!SDL_Init(SDL_INIT_VIDEO)){
        std::cout << "failed sdl init\n";
        return -1;
    }
    SDL_Window* window=SDL_CreateWindow("VOXEL_GE",width,height, SDL_WINDOW_RESIZABLE);
    if (!window){
        std::cout << "failed wimdow_creation\n";
        return -1;
    }
    bgfx::PlatformData pd{};
    SDL_PropertiesID props=SDL_GetWindowProperties(window);
    //#ifdef _WIN32
    pd.nwh=(void*)SDL_GetPointerProperty(props,SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr);
    bgfx::setPlatformData(pd);
    //#endif
    std::cout << pd.nwh << "\n";
    bgfx::Init init;
    init.type=bgfx::RendererType::Count;
    init.resolution.height=height;
    init.resolution.width=width;
    init.resolution.reset=BGFX_RESET_VSYNC;
    init.platformData.nwh=pd.nwh;
    bgfx::init(init);
    bgfx::ShaderHandle vsh=loadShader("C:\\Users\\vitop\\Desktop\\GAME_ENGINE\\src\\vs_voxel.bin");
    bgfx::ShaderHandle fsh=loadShader("C:\\Users\\vitop\\Desktop\\GAME_ENGINE\\src\\fs_voxel.bin");
    bgfx::ProgramHandle program=bgfx::createProgram(vsh,fsh,true);
    float view[16];
    bx::mtxLookAt(
        view,
        bx::Vec3{20.0f, 20.0f, 20.0f},   // camera position
        bx::Vec3{0.0f, 0.0f, 0.0f}       // look at
    );

    float proj[16];
    bx::mtxProj(
        proj,
        60.0f,
        float(width) / float(height),
        0.1f,
        1000.0f,
        bgfx::getCaps()->homogeneousDepth
    );

    bgfx::setViewTransform(0, view, proj);
    while (running){
        for (auto& [ChunkPos,chunk]:loaded_chunks){
            chunk.init(); 
            chunk.make_layout();
            if (!bgfx::isValid(chunk.vbh)){
                std::cout << "no_chunks\n";
                continue;
            }
            bgfx::setViewClear(0,BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, 0x303030ff);
            bgfx::setViewRect(0,0,0,width,height);
            /*bgfx::setState(
                BGFX_STATE_WRITE_RGB |
                BGFX_STATE_WRITE_Z |
                BGFX_STATE_DEPTH_TEST_LESS |
                BGFX_STATE_CULL_CCW//cw
            );*/
            bgfx::setVertexBuffer(0,chunk.vbh);
            bgfx::setIndexBuffer(chunk.ibh);
            bgfx::submit(0,program);
            bgfx::touch(0);
        }
        bgfx::frame();
        //input
    };
    SDL_Quit();
    return 0;
};
