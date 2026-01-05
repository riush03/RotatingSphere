#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

// OpenGL headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ImGui headers
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Simple math functions
struct Vec3 {
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    Vec3 operator/(float scalar) const { return Vec3(x / scalar, y / scalar, z / scalar); }
    
    float length() const { return sqrt(x*x + y*y + z*z); }
    Vec3 normalize() const { float l = length(); if(l > 0) return *this / l; return *this; }
    
    static float dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    static Vec3 cross(const Vec3& a, const Vec3& b) { 
        return Vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); 
    }
};

struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texcoord;
    Vec3 color;
    
    Vertex(Vec3 pos = Vec3(), Vec3 norm = Vec3(), Vec2 tex = Vec2(), Vec3 col = Vec3(1,1,1)) 
        : position(pos), normal(norm), texcoord(tex), color(col) {}
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Vec3 color;
    
    GLuint VAO, VBO, EBO;
    
    Mesh() : VAO(0), VBO(0), EBO(0), color(1.0f, 1.0f, 1.0f) {}
    
    void setupBuffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(0);
        
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(1);
        
        // Color attribute
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
    }
    
    void cleanup() {
        if (EBO) glDeleteBuffers(1, &EBO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (VAO) glDeleteVertexArrays(1, &VAO);
    }
    
    void draw() {
        if (VAO == 0) return;
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

struct Mat4 {
    float m[16];
    
    Mat4() {
        for(int i = 0; i < 16; i++) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    
    // Matrix multiplication operator
    Mat4 operator*(const Mat4& other) const {
        Mat4 result;
        
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i * 4 + j] = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    result.m[i * 4 + j] += m[i * 4 + k] * other.m[k * 4 + j];
                }
            }
        }
        
        return result;
    }
    
    static Mat4 identity() { 
        Mat4 result;
        for(int i = 0; i < 16; i++) result.m[i] = 0.0f;
        result.m[0] = result.m[5] = result.m[10] = result.m[15] = 1.0f;
        return result;
    }
    
    static Mat4 translate(float x, float y, float z) {
        Mat4 result = identity();
        result.m[12] = x;
        result.m[13] = y;
        result.m[14] = z;
        return result;
    }
    
    static Mat4 scale(float x, float y, float z) {
        Mat4 result = identity();
        result.m[0] = x;
        result.m[5] = y;
        result.m[10] = z;
        return result;
    }
    
    static Mat4 rotateX(float angle) {
        Mat4 result = identity();
        float c = cos(angle);
        float s = sin(angle);
        result.m[5] = c;
        result.m[6] = s;
        result.m[9] = -s;
        result.m[10] = c;
        return result;
    }
    
    static Mat4 rotateY(float angle) {
        Mat4 result = identity();
        float c = cos(angle);
        float s = sin(angle);
        result.m[0] = c;
        result.m[2] = -s;
        result.m[8] = s;
        result.m[10] = c;
        return result;
    }
    
    static Mat4 rotateZ(float angle) {
        Mat4 result = identity();
        float c = cos(angle);
        float s = sin(angle);
        result.m[0] = c;
        result.m[1] = s;
        result.m[4] = -s;
        result.m[5] = c;
        return result;
    }
    
    static Mat4 perspective(float fov, float aspect, float near, float far) {
        Mat4 result;
        for(int i = 0; i < 16; i++) result.m[i] = 0.0f;
        
        float f = 1.0f / tan(fov * 0.5f * 3.14159f / 180.0f);
        
        result.m[0] = f / aspect;
        result.m[5] = f;
        result.m[10] = (far + near) / (near - far);
        result.m[11] = -1.0f;
        result.m[14] = (2.0f * far * near) / (near - far);
        
        return result;
    }
    
    static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
        Mat4 result;
        
        Vec3 f = (center - eye).normalize();
        Vec3 s = Vec3::cross(f, up).normalize();
        Vec3 u = Vec3::cross(s, f);
        
        result.m[0] = s.x;
        result.m[1] = u.x;
        result.m[2] = -f.x;
        result.m[3] = 0.0f;
        
        result.m[4] = s.y;
        result.m[5] = u.y;
        result.m[6] = -f.y;
        result.m[7] = 0.0f;
        
        result.m[8] = s.z;
        result.m[9] = u.z;
        result.m[10] = -f.z;
        result.m[11] = 0.0f;
        
        result.m[12] = -Vec3::dot(s, eye);
        result.m[13] = -Vec3::dot(u, eye);
        result.m[14] = Vec3::dot(f, eye);
        result.m[15] = 1.0f;
        
        return result;
    }
};

// Vertex shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;
out vec3 LightPos;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    LightPos = lightPos;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

// Fragment shader source
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;
in vec3 LightPos;

out vec4 FragColor;

void main()
{
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(LightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    // Combine lighting with object color
    vec3 result = (ambient + diffuse + specular) * Color;
    FragColor = vec4(result, 1.0);
}
)";

// OBJ Loader Class
class OBJLoader {
public:
    static bool loadOBJ(const std::string& filepath, Mesh& mesh) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open OBJ file: " << filepath << std::endl;
            return false;
        }
        
        std::vector<Vec3> positions;
        std::vector<Vec3> normals;
        std::vector<Vec2> texcoords;
        
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;
            
            if (prefix == "v") { // Vertex position
                float x, y, z;
                iss >> x >> y >> z;
                positions.push_back(Vec3(x, y, z));
            }
            else if (prefix == "vn") { // Vertex normal
                float x, y, z;
                iss >> x >> y >> z;
                normals.push_back(Vec3(x, y, z));
            }
            else if (prefix == "vt") { // Texture coordinate
                float u, v;
                iss >> u >> v;
                texcoords.push_back(Vec2(u, v));
            }
            else if (prefix == "f") { // Face
                std::string vertex1, vertex2, vertex3;
                iss >> vertex1 >> vertex2 >> vertex3;
                
                // Parse each vertex (format: position/texcoord/normal)
                auto parseVertex = [&](const std::string& vertexStr) -> Vertex {
                    std::istringstream vss(vertexStr);
                    std::string posStr, texStr, normStr;
                    
                    std::getline(vss, posStr, '/');
                    std::getline(vss, texStr, '/');
                    std::getline(vss, normStr, '/');
                    
                    int posIdx = std::stoi(posStr) - 1;
                    int texIdx = texStr.empty() ? -1 : std::stoi(texStr) - 1;
                    int normIdx = normStr.empty() ? -1 : std::stoi(normStr) - 1;
                    
                    Vec3 pos = positions[posIdx];
                    Vec3 norm = (normIdx >= 0 && normIdx < normals.size()) ? normals[normIdx] : Vec3(0, 1, 0);
                    Vec2 tex = (texIdx >= 0 && texIdx < texcoords.size()) ? texcoords[texIdx] : Vec2(0, 0);
                    
                    // Generate a color based on position for visualization
                    Vec3 color = Vec3(
                        fabs(sin(pos.x * 2.0f)),
                        fabs(cos(pos.y * 2.0f)),
                        fabs(sin(pos.z * 2.0f))
                    );
                    
                    return Vertex(pos, norm, tex, color);
                };
                
                Vertex v1 = parseVertex(vertex1);
                Vertex v2 = parseVertex(vertex2);
                Vertex v3 = parseVertex(vertex3);
                
                // Add vertices and indices
                unsigned int baseIndex = vertices.size();
                vertices.push_back(v1);
                vertices.push_back(v2);
                vertices.push_back(v3);
                
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex + 2);
                
                // Check if there's a fourth vertex (quad face)
                if (!iss.eof()) {
                    std::string vertex4;
                    iss >> vertex4;
                    if (!vertex4.empty()) {
                        Vertex v4 = parseVertex(vertex4);
                        
                        // Split quad into two triangles
                        vertices.push_back(v4);
                        indices.push_back(baseIndex);
                        indices.push_back(baseIndex + 2);
                        indices.push_back(baseIndex + 3);
                    }
                }
            }
        }
        
        file.close();
        
        if (vertices.empty()) {
            std::cerr << "No vertices loaded from OBJ file" << std::endl;
            return false;
        }
        
        // Calculate normals if none were provided
        if (normals.empty()) {
            calculateNormals(vertices, indices);
        }
        
        mesh.vertices = vertices;
        mesh.indices = indices;
        
        std::cout << "Loaded OBJ: " << filepath << std::endl;
        std::cout << "  Vertices: " << vertices.size() << std::endl;
        std::cout << "  Indices: " << indices.size() << std::endl;
        std::cout << "  Triangles: " << indices.size() / 3 << std::endl;
        
        return true;
    }
    
private:
    static void calculateNormals(std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
        // Initialize normals to zero
        for (auto& vertex : vertices) {
            vertex.normal = Vec3(0, 0, 0);
        }
        
        // Calculate face normals and accumulate to vertices
        for (size_t i = 0; i < indices.size(); i += 3) {
            unsigned int i1 = indices[i];
            unsigned int i2 = indices[i + 1];
            unsigned int i3 = indices[i + 2];
            
            Vec3 v1 = vertices[i1].position;
            Vec3 v2 = vertices[i2].position;
            Vec3 v3 = vertices[i3].position;
            
            Vec3 edge1 = v2 - v1;
            Vec3 edge2 = v3 - v1;
            Vec3 normal = Vec3::cross(edge1, edge2).normalize();
            
            vertices[i1].normal = vertices[i1].normal + normal;
            vertices[i2].normal = vertices[i2].normal + normal;
            vertices[i3].normal = vertices[i3].normal + normal;
        }
        
        // Normalize all vertex normals
        for (auto& vertex : vertices) {
            vertex.normal = vertex.normal.normalize();
        }
    }
};

// Function to compile shaders
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
    }
    
    return shader;
}

// Function to create shader program
GLuint createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    GLint success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

// Function to send matrix to shader
void setShaderMat4(GLuint shader, const char* name, const Mat4& matrix) {
    GLint loc = glGetUniformLocation(shader, name);
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, matrix.m);
    }
}

// Function to send vector to shader
void setShaderVec3(GLuint shader, const char* name, const Vec3& vec) {
    GLint loc = glGetUniformLocation(shader, name);
    if (loc != -1) {
        glUniform3f(loc, vec.x, vec.y, vec.z);
    }
}

// Generate a simple house if OBJ file is not found
void generateSimpleHouse(Mesh& mesh) {
    // House dimensions
    float width = 4.0f;
    float height = 3.0f;
    float depth = 4.0f;
    float roofHeight = 2.0f;
    
    // Colors
    Vec3 wallColor(0.8f, 0.6f, 0.4f);    // Brown walls
    Vec3 roofColor(0.7f, 0.2f, 0.2f);    // Red roof
    Vec3 doorColor(0.5f, 0.35f, 0.25f);  // Dark brown door
    Vec3 windowColor(0.2f, 0.4f, 0.8f);  // Blue windows
    
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    // Front wall
    vertices.push_back(Vertex(Vec3(-width/2, 0, depth/2), Vec3(0,0,1), Vec2(0,0), wallColor));
    vertices.push_back(Vertex(Vec3(width/2, 0, depth/2), Vec3(0,0,1), Vec2(1,0), wallColor));
    vertices.push_back(Vertex(Vec3(width/2, height, depth/2), Vec3(0,0,1), Vec2(1,1), wallColor));
    vertices.push_back(Vertex(Vec3(-width/2, height, depth/2), Vec3(0,0,1), Vec2(0,1), wallColor));
    
    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(2); indices.push_back(3); indices.push_back(0);
    
    // Back wall
    vertices.push_back(Vertex(Vec3(-width/2, 0, -depth/2), Vec3(0,0,-1), Vec2(0,0), wallColor));
    vertices.push_back(Vertex(Vec3(width/2, 0, -depth/2), Vec3(0,0,-1), Vec2(1,0), wallColor));
    vertices.push_back(Vertex(Vec3(width/2, height, -depth/2), Vec3(0,0,-1), Vec2(1,1), wallColor));
    vertices.push_back(Vertex(Vec3(-width/2, height, -depth/2), Vec3(0,0,-1), Vec2(0,1), wallColor));
    
    indices.push_back(4); indices.push_back(5); indices.push_back(6);
    indices.push_back(6); indices.push_back(7); indices.push_back(4);
    
    // Left wall
    vertices.push_back(Vertex(Vec3(-width/2, 0, -depth/2), Vec3(-1,0,0), Vec2(0,0), wallColor));
    vertices.push_back(Vertex(Vec3(-width/2, 0, depth/2), Vec3(-1,0,0), Vec2(1,0), wallColor));
    vertices.push_back(Vertex(Vec3(-width/2, height, depth/2), Vec3(-1,0,0), Vec2(1,1), wallColor));
    vertices.push_back(Vertex(Vec3(-width/2, height, -depth/2), Vec3(-1,0,0), Vec2(0,1), wallColor));
    
    indices.push_back(8); indices.push_back(9); indices.push_back(10);
    indices.push_back(10); indices.push_back(11); indices.push_back(8);
    
    // Right wall
    vertices.push_back(Vertex(Vec3(width/2, 0, -depth/2), Vec3(1,0,0), Vec2(0,0), wallColor));
    vertices.push_back(Vertex(Vec3(width/2, 0, depth/2), Vec3(1,0,0), Vec2(1,0), wallColor));
    vertices.push_back(Vertex(Vec3(width/2, height, depth/2), Vec3(1,0,0), Vec2(1,1), wallColor));
    vertices.push_back(Vertex(Vec3(width/2, height, -depth/2), Vec3(1,0,0), Vec2(0,1), wallColor));
    
    indices.push_back(12); indices.push_back(13); indices.push_back(14);
    indices.push_back(14); indices.push_back(15); indices.push_back(12);
    
    // Roof (front triangle)
    vertices.push_back(Vertex(Vec3(-width/2, height, depth/2), Vec3(0,1,0), Vec2(0,0), roofColor));
    vertices.push_back(Vertex(Vec3(width/2, height, depth/2), Vec3(0,1,0), Vec2(1,0), roofColor));
    vertices.push_back(Vertex(Vec3(0, height + roofHeight, 0), Vec3(0,1,0), Vec2(0.5,1), roofColor));
    
    indices.push_back(16); indices.push_back(17); indices.push_back(18);
    
    // Roof (back triangle)
    vertices.push_back(Vertex(Vec3(-width/2, height, -depth/2), Vec3(0,1,0), Vec2(0,0), roofColor));
    vertices.push_back(Vertex(Vec3(width/2, height, -depth/2), Vec3(0,1,0), Vec2(1,0), roofColor));
    vertices.push_back(Vertex(Vec3(0, height + roofHeight, 0), Vec3(0,1,0), Vec2(0.5,1), roofColor));
    
    indices.push_back(19); indices.push_back(20); indices.push_back(18);
    
    // Roof (left side)
    vertices.push_back(Vertex(Vec3(-width/2, height, -depth/2), Vec3(-1,0.5,0), Vec2(0,0), roofColor));
    vertices.push_back(Vertex(Vec3(-width/2, height, depth/2), Vec3(-1,0.5,0), Vec2(1,0), roofColor));
    vertices.push_back(Vertex(Vec3(0, height + roofHeight, 0), Vec3(-1,0.5,0), Vec2(0.5,1), roofColor));
    
    indices.push_back(19); indices.push_back(16); indices.push_back(18);
    
    // Roof (right side)
    vertices.push_back(Vertex(Vec3(width/2, height, -depth/2), Vec3(1,0.5,0), Vec2(0,0), roofColor));
    vertices.push_back(Vertex(Vec3(width/2, height, depth/2), Vec3(1,0.5,0), Vec2(1,0), roofColor));
    vertices.push_back(Vertex(Vec3(0, height + roofHeight, 0), Vec3(1,0.5,0), Vec2(0.5,1), roofColor));
    
    indices.push_back(20); indices.push_back(17); indices.push_back(18);
    
    // Door
    vertices.push_back(Vertex(Vec3(-0.5f, 0, depth/2 + 0.01f), Vec3(0,0,1), Vec2(0,0), doorColor));
    vertices.push_back(Vertex(Vec3(0.5f, 0, depth/2 + 0.01f), Vec3(0,0,1), Vec2(1,0), doorColor));
    vertices.push_back(Vertex(Vec3(0.5f, 2.0f, depth/2 + 0.01f), Vec3(0,0,1), Vec2(1,1), doorColor));
    vertices.push_back(Vertex(Vec3(-0.5f, 2.0f, depth/2 + 0.01f), Vec3(0,0,1), Vec2(0,1), doorColor));
    
    indices.push_back(24); indices.push_back(25); indices.push_back(26);
    indices.push_back(26); indices.push_back(27); indices.push_back(24);
    
    // Windows
    // Left window
    vertices.push_back(Vertex(Vec3(-width/2 + 0.01f, 1.5f, 0.5f), Vec3(-1,0,0), Vec2(0,0), windowColor));
    vertices.push_back(Vertex(Vec3(-width/2 + 0.01f, 1.5f, -0.5f), Vec3(-1,0,0), Vec2(1,0), windowColor));
    vertices.push_back(Vertex(Vec3(-width/2 + 0.01f, 2.5f, -0.5f), Vec3(-1,0,0), Vec2(1,1), windowColor));
    vertices.push_back(Vertex(Vec3(-width/2 + 0.01f, 2.5f, 0.5f), Vec3(-1,0,0), Vec2(0,1), windowColor));
    
    indices.push_back(28); indices.push_back(29); indices.push_back(30);
    indices.push_back(30); indices.push_back(31); indices.push_back(28);
    
    // Right window
    vertices.push_back(Vertex(Vec3(width/2 - 0.01f, 1.5f, 0.5f), Vec3(1,0,0), Vec2(0,0), windowColor));
    vertices.push_back(Vertex(Vec3(width/2 - 0.01f, 1.5f, -0.5f), Vec3(1,0,0), Vec2(1,0), windowColor));
    vertices.push_back(Vertex(Vec3(width/2 - 0.01f, 2.5f, -0.5f), Vec3(1,0,0), Vec2(1,1), windowColor));
    vertices.push_back(Vertex(Vec3(width/2 - 0.01f, 2.5f, 0.5f), Vec3(1,0,0), Vec2(0,1), windowColor));
    
    indices.push_back(32); indices.push_back(33); indices.push_back(34);
    indices.push_back(34); indices.push_back(35); indices.push_back(32);
    
    // Floor
    vertices.push_back(Vertex(Vec3(-width/2 - 2, -0.01f, -depth/2 - 2), Vec3(0,1,0), Vec2(0,0), Vec3(0.3f, 0.6f, 0.3f)));
    vertices.push_back(Vertex(Vec3(width/2 + 2, -0.01f, -depth/2 - 2), Vec3(0,1,0), Vec2(1,0), Vec3(0.3f, 0.6f, 0.3f)));
    vertices.push_back(Vertex(Vec3(width/2 + 2, -0.01f, depth/2 + 2), Vec3(0,1,0), Vec2(1,1), Vec3(0.3f, 0.6f, 0.3f)));
    vertices.push_back(Vertex(Vec3(-width/2 - 2, -0.01f, depth/2 + 2), Vec3(0,1,0), Vec2(0,1), Vec3(0.3f, 0.6f, 0.3f)));
    
    indices.push_back(36); indices.push_back(37); indices.push_back(38);
    indices.push_back(38); indices.push_back(39); indices.push_back(36);
    
    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.color = Vec3(1.0f, 1.0f, 1.0f);
}

int main() {
    std::cout << "Initializing 3D House Viewer..." << std::endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed!" << std::endl;
        return -1;
    }
    
    // Configure GLFW window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    // Create GLFW window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "3D House Viewer", NULL, NULL);
    if (!window) {
        std::cerr << "GLFW window creation failed!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Print OpenGL info
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLFW Version: " << glfwGetVersionString() << std::endl;
    std::cout << "GLEW Version: " << glewGetString(GLEW_VERSION) << std::endl;
    
    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Enable OpenGL features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Create shader program
    GLuint shaderProgram = createShaderProgram();
    
    // Try to load OBJ house file
    Mesh houseMesh;
    std::string objPath = "assets/models/obj/cottage_obj.obj";
    bool objLoaded = false;
    
    if (std::ifstream(objPath).good()) {
        std::cout << "Found OBJ file at: " << objPath << std::endl;
        objLoaded = OBJLoader::loadOBJ(objPath, houseMesh);
    } else {
        std::cout << "OBJ file not found at: " << objPath << std::endl;
        std::cout << "Generating a simple house model..." << std::endl;
        generateSimpleHouse(houseMesh);
        objLoaded = true;
    }
    
    if (!objLoaded) {
        std::cerr << "Failed to load or generate house model!" << std::endl;
        return -1;
    }
    
    // Setup mesh buffers
    houseMesh.setupBuffers();
    std::cout << "House mesh ready with " << houseMesh.vertices.size() << " vertices and " 
              << houseMesh.indices.size() << " indices" << std::endl;
    
    // Application state
    bool showImGui = true;
    float rotationSpeed = 30.0f;
    float cameraDistance = 10.0f;
    float cameraHeight = 3.0f;
    float cameraAngle = 45.0f;
    Vec3 lightPos = Vec3(5.0f, 10.0f, 5.0f);
    float backgroundColor[3] = {0.53f, 0.81f, 0.98f}; // Sky blue
    bool wireframeMode = false;
    bool rotateHouse = true;
    float houseScale = 1.0f;
    Vec3 housePosition = Vec3(0.0f, 0.0f, 0.0f);
    bool showAxes = true;
    
    // Performance tracking
    float lastTime = 0.0f;
    float deltaTime = 0.0f;
    int frameCount = 0;
    float fpsTime = 0.0f;
    float fps = 0.0f;
    
    std::cout << "\nStarting render loop..." << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "- ESC: Exit" << std::endl;
    std::cout << "- M: Toggle ImGui Menu" << std::endl;
    std::cout << "- W: Toggle Wireframe" << std::endl;
    std::cout << "- R: Reset Camera" << std::endl;
    
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Calculate FPS
        frameCount++;
        fpsTime += deltaTime;
        if (fpsTime >= 1.0f) {
            fps = frameCount / fpsTime;
            frameCount = 0;
            fpsTime = 0.0f;
        }
        
        // Handle keyboard input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            static double lastPress = 0.0;
            if (currentTime - lastPress > 0.3) {
                showImGui = !showImGui;
                lastPress = currentTime;
            }
        }
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            static double lastPress = 0.0;
            if (currentTime - lastPress > 0.3) {
                wireframeMode = !wireframeMode;
                lastPress = currentTime;
            }
        }
        
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            static double lastPress = 0.0;
            if (currentTime - lastPress > 0.3) {
                cameraDistance = 10.0f;
                cameraHeight = 3.0f;
                cameraAngle = 45.0f;
                houseScale = 1.0f;
                housePosition = Vec3(0, 0, 0);
                lastPress = currentTime;
            }
        }
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // ImGui menu window
        if (showImGui) {
            ImGui::Begin("3D House Viewer Controls", &showImGui, ImGuiWindowFlags_AlwaysAutoResize);
            
            // Performance info
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
            ImGui::Text("Vertices: %zu", houseMesh.vertices.size());
            ImGui::Text("Triangles: %zu", houseMesh.indices.size() / 3);
            ImGui::Separator();
            
            // Camera controls
            ImGui::Text("Camera Settings:");
            ImGui::SliderFloat("Distance", &cameraDistance, 1.0f, 50.0f);
            ImGui::SliderFloat("Height", &cameraHeight, -10.0f, 20.0f);
            ImGui::SliderFloat("Angle", &cameraAngle, 0.0f, 360.0f);
            
            // House controls
            ImGui::Separator();
            ImGui::Text("House Settings:");
            ImGui::Checkbox("Auto Rotate", &rotateHouse);
            ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 180.0f);
            ImGui::SliderFloat("Scale", &houseScale, 0.1f, 5.0f);
            ImGui::SliderFloat3("Position", &housePosition.x, -10.0f, 10.0f);
            
            // Lighting controls
            ImGui::Separator();
            ImGui::Text("Lighting:");
            ImGui::SliderFloat3("Light Position", &lightPos.x, -20.0f, 20.0f);
            
            // Display settings
            ImGui::Separator();
            ImGui::Text("Display:");
            ImGui::Checkbox("Wireframe Mode", &wireframeMode);
            ImGui::Checkbox("Show Coordinate Axes", &showAxes);
            ImGui::ColorEdit3("Sky Color", backgroundColor);
            
            // Model info
            ImGui::Separator();
            ImGui::Text("Model Information:");
            if (objPath == "assets/models/obj/cottage_obj.obj") {
                ImGui::Text("Loaded from: %s", objPath.c_str());
            } else {
                ImGui::Text("Generated: Simple House Model");
            }
            
            // Reset button
            ImGui::Separator();
            if (ImGui::Button("Reset to Defaults")) {
                rotationSpeed = 30.0f;
                cameraDistance = 10.0f;
                cameraHeight = 3.0f;
                cameraAngle = 45.0f;
                lightPos = Vec3(5.0f, 10.0f, 5.0f);
                backgroundColor[0] = 0.53f;
                backgroundColor[1] = 0.81f;
                backgroundColor[2] = 0.98f;
                houseScale = 1.0f;
                housePosition = Vec3(0, 0, 0);
                showAxes = true;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Exit")) {
                glfwSetWindowShouldClose(window, true);
            }
            
            ImGui::Separator();
            ImGui::Text("Controls:");
            ImGui::BulletText("ESC: Exit");
            ImGui::BulletText("M: Toggle Menu");
            ImGui::BulletText("W: Toggle Wireframe");
            ImGui::BulletText("R: Reset Camera");
            
            ImGui::End();
        }
        
        // Get window size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        
        // Clear screen
        glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set wireframe mode
        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.5f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        
        // Calculate camera position
        float camAngleRad = cameraAngle * 3.14159f / 180.0f;
        Vec3 cameraPos = Vec3(
            cameraDistance * cos(camAngleRad),
            cameraHeight,
            cameraDistance * sin(camAngleRad)
        );
        
        // Create matrices
        Mat4 projection = Mat4::perspective(60.0f, (float)width / (float)height, 0.1f, 100.0f);
        Mat4 view = Mat4::lookAt(cameraPos, housePosition, Vec3(0.0f, 1.0f, 0.0f));
        
        // Create model matrix for house
        static float rotationAngle = 0.0f;
        if (rotateHouse) {
            rotationAngle += rotationSpeed * deltaTime * 3.14159f / 180.0f;
        }
        
        Mat4 model = Mat4::translate(housePosition.x, housePosition.y, housePosition.z);
        Mat4 rotation = Mat4::rotateY(rotationAngle);
        Mat4 scale = Mat4::scale(houseScale, houseScale, houseScale);
        model = model * rotation * scale;
        
        // Use shader program
        glUseProgram(shaderProgram);
        
        // Pass matrices to shader
        setShaderMat4(shaderProgram, "model", model);
        setShaderMat4(shaderProgram, "view", view);
        setShaderMat4(shaderProgram, "projection", projection);
        setShaderVec3(shaderProgram, "lightPos", lightPos);
        
        // Draw house
        houseMesh.draw();
        
        // Draw coordinate axes if enabled
        if (showAxes) {
            glUseProgram(0); // Use fixed pipeline for axes
            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf(projection.m);
            glMatrixMode(GL_MODELVIEW);
            Mat4 mv = view * model;
            glLoadMatrixf(mv.m);
            
            glBegin(GL_LINES);
            // X axis - red
            glColor3f(1.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(5.0f, 0.0f, 0.0f);
            
            // Y axis - green
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 5.0f, 0.0f);
            
            // Z axis - blue
            glColor3f(0.0f, 0.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 5.0f);
            glEnd();
        }
        
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    std::cout << "\nCleaning up resources..." << std::endl;
    
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    // Cleanup mesh
    houseMesh.cleanup();
    
    // Cleanup shader
    glDeleteProgram(shaderProgram);
    
    // Destroy window and terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "Program exited successfully" << std::endl;
    return 0;
}