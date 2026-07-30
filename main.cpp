#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <cstdint>
#include <imgui.h>
#include <unordered_map>
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
        {1,1,0,1,0},
        {0,1,1,0,1},
        {1,1,1,1,1}
    }},

    // Bottom
    {{
        {0,0,1,0,0},
        {1,0,1,1,0},
        {0,0,0,0,1},
        {1,0,0,1,1}
    }},

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
    ChunkPos position;
    public:
        std::vector<Block> blocks;
        Mesh mesh;
        bool dirty = false;

        Chunk(ChunkPos pos)
            : position(pos){
            blocks.resize(chunk_size * chunk_height * chunk_size);
        }

        Block& getBlock(int x,int y,int z)
        {
            return blocks[index(x,y,z)];
        }
    void make_mesh(){
        mesh.vertices.clear();
        mesh.indices.clear();
        for (int x=0;x<chunk_size; x++){
            for (int y=0;y<chunk_height; y++){
                for (int z=0; z<chunk_size; z++){
                    Block& block=blocks[index(x,y,z)];
                    //std::vector<std::array<int,3>> faces_to_show_in_this_block;
                    if (!block.isSolid()){
                        continue;
                    }
                    int global_blockX = position.x * chunk_size + x;
                    int global_blockZ = position.z * chunk_size + z;
                    //x+
                    if (isAir(global_blockX+1,y,global_blockZ)){
                        //faces_to_show_in_this_block.push_back({x+1,y,z})
                        mesh.addFace(2,x,y,z);
                    }
                    //x-
                    if (isAir(global_blockX-1,y,global_blockZ)){
                        //faces_to_show_in_this_block.push_back({x-1,y,z})
                        mesh.addFace(3,x,y,z);
                    }
                    //z+
                    if (isAir(global_blockX,y,global_blockZ+1)){
                        //faces_to_show_in_this_block.push_back({x,y,z+1})
                        mesh.addFace(4,x,y,z);
                    }
                    //z-
                    if (isAir(global_blockX,y,global_blockZ-1)){
                        //faces_to_show_in_this_block.push_back({x,y,z-1})
                        mesh.addFace(5,x,y,z);
                    }
                    //y+
                    if ((isAir(global_blockX,y+1,global_blockZ))){
                        //faces_to_show_in_this_block.push_back({x,y+1,z})
                        mesh.addFace(0,x,y,z);
                    }
                    //y-
                    if (isAir(global_blockX,y-1,global_blockZ)){
                        //faces_to_show_in_this_block.push_back({x,y-1,z})
                        mesh.addFace(1,x,y,z);
                    }
                }
            }
        }
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
int main(){
    return 0;
}
