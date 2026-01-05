#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <random>
#include <algorithm>
#include <memory>

// OpenGL headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ImGui headers
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Random number generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dis(0.0f, 1.0f);

// Math structures
struct Vec3 {
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
    Vec3 operator/(float scalar) const { 
        if (fabs(scalar) > 0.0001f) return Vec3(x / scalar, y / scalar, z / scalar);
        return Vec3(0, 0, 0);
    }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    
    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vec3& operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vec3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
    
    float length() const { return sqrt(x*x + y*y + z*z); }
    float lengthSquared() const { return x*x + y*y + z*z; }
    Vec3 normalize() const { 
        float l = length(); 
        if(l > 0.0001f) return *this / l; 
        return Vec3(0, 1, 0);
    }
    
    static float dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    static Vec3 cross(const Vec3& a, const Vec3& b) { 
        return Vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); 
    }
    
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a * (1.0f - t) + b * t;
    }
};

struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
    
    float length() const { return sqrt(x*x + y*y); }
    Vec2 normalize() const { 
        float l = length(); 
        if(l > 0.0001f) return Vec2(x / l, y / l);
        return Vec2(0, 0);
    }
    
    Vec2 operator/(float scalar) const {
        if (fabs(scalar) > 0.0001f) return Vec2(x / scalar, y / scalar);
        return Vec2(0, 0);
    }
    
    Vec2 operator*(float scalar) const { 
        return Vec2(x * scalar, y * scalar);
    }
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec3 color;
    
    Vertex(Vec3 pos = Vec3(), Vec3 norm = Vec3(), Vec3 col = Vec3(1,1,1)) 
        : position(pos), normal(norm), color(col) {}
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    GLuint VAO, VBO, EBO;
    
    Mesh() : VAO(0), VBO(0), EBO(0) {}
    
    void setupBuffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        
        if (!indices.empty()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        }
        
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
        if (!indices.empty()) {
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        }
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

// Game Objects
class MetaBall {
public:
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    float radius;
    Vec3 color;
    float rotationAngle;
    float rotationSpeed;
    float mass;
    float elasticity;
    float health;
    bool isAlive;
    
    MetaBall() : position(0, 2, 0), velocity(0, 0, 0), acceleration(0, 0, 0),
                radius(0.5f), color(0.2f, 0.8f, 0.9f), rotationAngle(0),  // Cyan-blue color
                rotationSpeed(2.0f), mass(1.0f), elasticity(0.8f), health(100.0f), isAlive(true) {}
    
    void update(float deltaTime, const Vec3& gravity) {
        if (!isAlive) return;
        
        // Apply gravity
        acceleration = gravity;
        
        // Update velocity and position
        velocity += acceleration * deltaTime;
        position += velocity * deltaTime;
        
        // Update rotation
        rotationAngle += rotationSpeed * deltaTime;
        if (rotationAngle > 360.0f) rotationAngle -= 360.0f;
        
        // Simple ground collision
        if (position.y - radius < 0) {
            position.y = radius;
            velocity.y = -velocity.y * elasticity;
            velocity *= 0.95f; // Friction
        }
        
        // Keep ball within bounds
        if (position.x < -10) position.x = -10;
        if (position.x > 10) position.x = 10;
        if (position.z < -50) position.z = -50;
    }
    
    void applyForce(const Vec3& force) {
        acceleration += force / mass;
    }
    
    void takeDamage(float damage) {
        health -= damage;
        if (health <= 0) {
            isAlive = false;
        }
    }
    
    Mat4 getModelMatrix() const {
        Mat4 model = Mat4::translate(position.x, position.y, position.z);
        model = model * Mat4::rotateY(rotationAngle);
        model = model * Mat4::rotateX(rotationAngle * 0.7f);
        model = model * Mat4::scale(radius, radius, radius);
        return model;
    }
};

class Obstacle {
public:
    Vec3 position;
    float width, height, depth;
    Vec3 color;
    bool isActive;
    float damage;
    int type; // 0: cube, 1: pyramid, 2: cylinder
    
    Obstacle() : position(0, 0, 0), width(1.0f), height(1.0f), depth(1.0f),
                color(0.9f, 0.3f, 0.1f), isActive(true), damage(10.0f), type(0) {}  // Orange-red color
    
    bool checkCollision(const MetaBall& ball) const {
        if (!isActive) return false;
        
        // Simple sphere-AABB collision
        Vec3 closestPoint;
        closestPoint.x = std::max(position.x - width/2, std::min(ball.position.x, position.x + width/2));
        closestPoint.y = std::max(position.y, std::min(ball.position.y, position.y + height));
        closestPoint.z = std::max(position.z - depth/2, std::min(ball.position.z, position.z + depth/2));
        
        float distance = (closestPoint - ball.position).length();
        return distance < ball.radius;
    }
    
    Mat4 getModelMatrix() const {
        Mat4 model = Mat4::translate(position.x, position.y + height/2, position.z);
        return model * Mat4::scale(width, height, depth);
    }
};

class Tree {
public:
    Vec3 position;
    float height;
    float trunkRadius;
    float foliageRadius;
    Vec3 trunkColor;
    Vec3 foliageColor;
    
    Tree() : position(0, 0, 0), height(3.0f), trunkRadius(0.2f), 
            foliageRadius(1.5f), trunkColor(0.4f, 0.2f, 0.1f), 
            foliageColor(0.1f, 0.5f, 0.2f) {}
    
    Mat4 getTrunkModelMatrix() const {
        Mat4 model = Mat4::translate(position.x, position.y + height/2, position.z);
        return model * Mat4::scale(trunkRadius, height, trunkRadius);
    }
    
    Mat4 getFoliageModelMatrix() const {
        Mat4 model = Mat4::translate(position.x, position.y + height, position.z);
        return model * Mat4::scale(foliageRadius, foliageRadius * 0.8f, foliageRadius);
    }
};

class Terrain {
private:
    std::vector<float> heightMap;
    int width, depth;
    float gridSize;
    
public:
    Terrain(int w = 100, int d = 200, float grid = 1.0f) : width(w), depth(d), gridSize(grid) {
        generateHeightMap();
    }
    
    void generateHeightMap() {
        heightMap.resize(width * depth);
        
        // Generate terrain using multiple octaves of noise
        for (int z = 0; z < depth; z++) {
            for (int x = 0; x < width; x++) {
                float height = 0;
                
                // Base terrain
                height += sin(x * 0.1f) * cos(z * 0.1f) * 0.5f;
                
                // Add hills
                height += sin(x * 0.05f + z * 0.03f) * 0.3f;
                
                // Add road (flat path in the middle)
                float roadWidth = 4.0f;
                float distanceFromCenter = fabs(x - width/2);
                if (distanceFromCenter < roadWidth) {
                    height = 0.1f; // Flat road
                } else {
                    // Add ditches
                    height -= (distanceFromCenter - roadWidth) * 0.1f;
                }
                
                // Add some random bumps
                height += (dis(gen) - 0.5f) * 0.1f;
                
                heightMap[z * width + x] = height;
            }
        }
    }
    
    float getHeight(float x, float z) const {
        int xi = static_cast<int>((x + width/2) / gridSize);
        int zi = static_cast<int>((z + depth/2) / gridSize);
        
        if (xi < 0 || xi >= width - 1 || zi < 0 || zi >= depth - 1) {
            return 0.0f;
        }
        
        // Bilinear interpolation
        float xRatio = fmod(x + width/2, gridSize) / gridSize;
        float zRatio = fmod(z + depth/2, gridSize) / gridSize;
        
        float h1 = heightMap[zi * width + xi];
        float h2 = heightMap[zi * width + xi + 1];
        float h3 = heightMap[(zi + 1) * width + xi];
        float h4 = heightMap[(zi + 1) * width + xi + 1];
        
        float top = h1 * (1 - xRatio) + h2 * xRatio;
        float bottom = h3 * (1 - xRatio) + h4 * xRatio;
        
        return top * (1 - zRatio) + bottom * zRatio;
    }
    
    Vec3 getNormal(float x, float z) const {
        float eps = 0.1f;
        float hL = getHeight(x - eps, z);
        float hR = getHeight(x + eps, z);
        float hD = getHeight(x, z - eps);
        float hU = getHeight(x, z + eps);
        
        Vec3 normal = Vec3(hL - hR, 2.0f * eps, hD - hU);
        return normal.normalize();
    }
    
    Mesh generateMesh() {
        Mesh mesh;
        
        // Generate vertices
        for (int z = 0; z < depth - 1; z++) {
            for (int x = 0; x < width - 1; x++) {
                // Create two triangles for each grid square
                float worldX = (x - width/2) * gridSize;
                float worldZ = (z - depth/2) * gridSize;
                
                // Vertex colors based on height and position
                Vec3 grassColor(0.1f, 0.7f, 0.1f);  // Bright green grass
                Vec3 roadColor(0.3f, 0.3f, 0.35f);  // Gray road
                Vec3 dirtColor(0.5f, 0.4f, 0.2f);   // Brown dirt
                
                // Check if on road
                float distanceFromCenter = fabs(x - width/2);
                Vec3 color;
                if (distanceFromCenter < 4.0f) {
                    color = roadColor;
                } else if (getHeight(worldX, worldZ) > 0.2f) {
                    color = grassColor;
                } else {
                    color = dirtColor;
                }
                
                // Triangle 1
                Vec3 p1(worldX, getHeight(worldX, worldZ), worldZ);
                Vec3 p2(worldX + gridSize, getHeight(worldX + gridSize, worldZ), worldZ);
                Vec3 p3(worldX, getHeight(worldX, worldZ + gridSize), worldZ + gridSize);
                
                Vec3 n1 = getNormal(worldX, worldZ);
                Vec3 n2 = getNormal(worldX + gridSize, worldZ);
                Vec3 n3 = getNormal(worldX, worldZ + gridSize);
                
                mesh.vertices.push_back(Vertex(p1, n1, color));
                mesh.vertices.push_back(Vertex(p2, n2, color));
                mesh.vertices.push_back(Vertex(p3, n3, color));
                
                // Triangle 2
                Vec3 p4(worldX + gridSize, getHeight(worldX + gridSize, worldZ), worldZ);
                Vec3 p5(worldX + gridSize, getHeight(worldX + gridSize, worldZ + gridSize), worldZ + gridSize);
                Vec3 p6(worldX, getHeight(worldX, worldZ + gridSize), worldZ + gridSize);
                
                Vec3 n4 = getNormal(worldX + gridSize, worldZ);
                Vec3 n5 = getNormal(worldX + gridSize, worldZ + gridSize);
                Vec3 n6 = getNormal(worldX, worldZ + gridSize);
                
                mesh.vertices.push_back(Vertex(p4, n4, color));
                mesh.vertices.push_back(Vertex(p5, n5, color));
                mesh.vertices.push_back(Vertex(p6, n6, color));
            }
        }
        
        return mesh;
    }
};

class Game {
private:
    MetaBall player;
    Terrain terrain;
    std::vector<Obstacle> obstacles;
    std::vector<Vec3> collectibles;
    std::vector<Tree> trees;
    std::vector<Vec3> grassPatches;
    
    float gameSpeed;
    float difficulty;
    float score;
    float distance;
    bool gameOver;
    bool gamePaused;
    bool inMenu;
    
    // Camera
    Vec3 cameraPosition;
    Vec3 cameraTarget;
    float cameraAngle;
    float cameraDistance;
    
    // Environment rotation
    float environmentRotation;
    float environmentRotationSpeed;

public:
    // Game state enum - MADE PUBLIC
    enum GameState { MENU, PLAYING, GAME_OVER, PAUSED };
    
private:
    GameState currentState;  // This stays private

public:
    Game() : gameSpeed(10.0f), difficulty(1.0f), score(0), distance(0), 
            gameOver(false), gamePaused(false), inMenu(true),
            cameraAngle(0), cameraDistance(8.0f), currentState(MENU),
            environmentRotation(0), environmentRotationSpeed(0.1f) {
        
        resetGame();
        
        // Initial camera setup
        cameraPosition = Vec3(0, 5, 10);
        cameraTarget = player.position;
    }
    
    void resetGame() {
        player = MetaBall();
        terrain = Terrain(100, 200, 1.0f);
        obstacles.clear();
        collectibles.clear();
        trees.clear();
        grassPatches.clear();
        
        gameSpeed = 10.0f;
        difficulty = 1.0f;
        score = 0;
        distance = 0;
        gameOver = false;
        gamePaused = false;
        environmentRotation = 0;
        currentState = PLAYING;
        
        // Generate initial obstacles, collectibles, and environment
        generateObstacles(20);
        generateCollectibles(10);
        generateTrees(30);
        generateGrass(50);
    }
    
    void generateObstacles(int count) {
        for (int i = 0; i < count; i++) {
            Obstacle obs;
            obs.type = rand() % 3;
            
            // Random position on road
            obs.position.x = (dis(gen) - 0.5f) * 3.0f; // Within road width
            obs.position.z = -30.0f - i * 15.0f; // Spread out
            
            // Random size
            obs.width = 0.5f + dis(gen) * 1.0f;
            obs.height = 0.5f + dis(gen) * 2.0f;
            obs.depth = 0.5f + dis(gen) * 1.0f;
            
            // Orange-red color for obstacles
            obs.color = Vec3(0.9f, 0.3f + dis(gen) * 0.2f, 0.1f + dis(gen) * 0.1f);
            
            obs.damage = 10.0f + dis(gen) * 10.0f;
            obstacles.push_back(obs);
        }
    }
    
    void generateCollectibles(int count) {
        for (int i = 0; i < count; i++) {
            Vec3 collectible;
            collectible.x = (dis(gen) - 0.5f) * 6.0f;
            collectible.y = 1.0f + dis(gen) * 2.0f;
            collectible.z = -20.0f - i * 10.0f;
            collectibles.push_back(collectible);
        }
    }
    
    void generateTrees(int count) {
        for (int i = 0; i < count; i++) {
            Tree tree;
            int width = 200;
            
            // Position trees on the sides of the road
            float side = (dis(gen) > 0.5f) ? 1.0f : -1.0f;
            tree.position.x = (width/2 + 2.0f + dis(gen) * 10.0f) * side;
            tree.position.z = -20.0f - i * 20.0f;
            tree.position.y = terrain.getHeight(tree.position.x, tree.position.z);
            
            // Randomize tree properties
            tree.height = 2.0f + dis(gen) * 4.0f;
            tree.trunkRadius = 0.1f + dis(gen) * 0.2f;
            tree.foliageRadius = 0.8f + dis(gen) * 1.2f;
            
            // Randomize foliage color (different shades of green)
            tree.foliageColor = Vec3(0.0f + dis(gen) * 0.2f, 0.4f + dis(gen) * 0.4f, 0.0f + dis(gen) * 0.2f);
            
            trees.push_back(tree);
        }
    }
    
    void generateGrass(int count) {
        for (int i = 0; i < count; i++) {
            Vec3 grassPatch;
            int width = 200;
            
            // Position grass patches on the sides of the road
            float side = (dis(gen) > 0.5f) ? 1.0f : -1.0f;
            grassPatch.x = (width/2 + 1.0f + dis(gen) * 8.0f) * side;
            grassPatch.z = -10.0f - i * 8.0f;
            grassPatch.y = terrain.getHeight(grassPatch.x, grassPatch.z) + 0.1f;
            
            grassPatches.push_back(grassPatch);
        }
    }
    
    void update(float deltaTime) {
        if (currentState != PLAYING) return;
        
        // Update environment rotation
        environmentRotation += environmentRotationSpeed * deltaTime;
        if (environmentRotation > 360.0f) environmentRotation -= 360.0f;
        
        // Update player
        Vec3 gravity(0, -9.8f, 0);
        player.update(deltaTime, gravity);
        
        // Terrain collision
        float terrainHeight = terrain.getHeight(player.position.x, player.position.z);
        if (player.position.y - player.radius < terrainHeight) {
            player.position.y = terrainHeight + player.radius;
            player.velocity.y = 0;
            
            // Apply terrain normal for sliding
            Vec3 normal = terrain.getNormal(player.position.x, player.position.z);
            player.velocity = player.velocity - normal * Vec3::dot(player.velocity, normal) * 0.1f;
        }
        
        // Update game progression
        distance += gameSpeed * deltaTime;
        score += gameSpeed * deltaTime * 0.1f;
        
        // Increase difficulty over time
        difficulty += deltaTime * 0.01f;
        gameSpeed += deltaTime * 0.1f;
        
        // Check collisions with obstacles
        for (auto& obs : obstacles) {
            if (obs.checkCollision(player)) {
                player.takeDamage(obs.damage);
                player.velocity = player.velocity * -0.5f; // Bounce back
                obs.isActive = false;
                
                // Score penalty
                score -= obs.damage * 2;
                if (score < 0) score = 0;
            }
        }
        
        // Check collectibles
        for (auto it = collectibles.begin(); it != collectibles.end(); ) {
            float dist = (player.position - *it).length();
            if (dist < player.radius + 0.5f) {
                score += 50;
                player.health = std::min(player.health + 10.0f, 100.0f);
                it = collectibles.erase(it);
            } else {
                ++it;
            }
        }
        
        // Check game over
        if (!player.isAlive || player.health <= 0) {
            currentState = GAME_OVER;
            gameOver = true;
        }
        
        // Update camera
        updateCamera(deltaTime);
        
        // Generate more obstacles, trees, and grass as player progresses
        if (obstacles.back().position.z > player.position.z - 100) {
            generateObstacles(5);
        }
        
        if (collectibles.empty() || collectibles.back().z > player.position.z - 80) {
            generateCollectibles(3);
        }
        
        if (trees.back().position.z > player.position.z - 150) {
            generateTrees(5);
        }
        
        if (grassPatches.back().z > player.position.z - 100) {
            generateGrass(10);
        }
    }
    
    void updateCamera(float deltaTime) {
        // Third-person camera following the ball
        cameraAngle += deltaTime * 0.5f;
        
        // Camera follows behind the ball
        float behindDistance = 8.0f;
        float height = 4.0f;
        
        cameraTarget = Vec3::lerp(cameraTarget, player.position, deltaTime * 5.0f);
        
        // Calculate camera position
        Vec3 offset = Vec3(
            sin(cameraAngle) * cameraDistance,
            height,
            cos(cameraAngle) * cameraDistance
        );
        
        cameraPosition = cameraTarget + offset;
    }
    
    void handleInput(int key, int action) {
        if (currentState == PLAYING && action == GLFW_PRESS) {
            float forceStrength = 15.0f;
            
            switch(key) {
                case GLFW_KEY_W:
                case GLFW_KEY_UP:
                    player.applyForce(Vec3(0, 0, -forceStrength));
                    break;
                case GLFW_KEY_S:
                case GLFW_KEY_DOWN:
                    player.applyForce(Vec3(0, 0, forceStrength));
                    break;
                case GLFW_KEY_A:
                case GLFW_KEY_LEFT:
                    player.applyForce(Vec3(-forceStrength, 0, 0));
                    break;
                case GLFW_KEY_D:
                case GLFW_KEY_RIGHT:
                    player.applyForce(Vec3(forceStrength, 0, 0));
                    break;
                case GLFW_KEY_SPACE:
                    if (player.position.y - player.radius <= terrain.getHeight(player.position.x, player.position.z) + 0.1f) {
                        player.velocity.y = 8.0f;
                    }
                    break;
                case GLFW_KEY_ESCAPE:
                    togglePause();
                    break;
                case GLFW_KEY_R:
                    // Toggle environment rotation
                    environmentRotationSpeed = (environmentRotationSpeed == 0) ? 0.1f : 0.0f;
                    break;
            }
        }
    }
    
    void togglePause() {
        if (currentState == PLAYING) {
            currentState = PAUSED;
            gamePaused = true;
        } else if (currentState == PAUSED) {
            currentState = PLAYING;
            gamePaused = false;
        }
    }
    
    void startGame() {
        resetGame();
        currentState = PLAYING;
        inMenu = false;
    }
    
    void returnToMenu() {
        currentState = MENU;
        inMenu = true;
    }
    
    // Getters
    MetaBall& getPlayer() { return player; }
    Terrain& getTerrain() { return terrain; }
    std::vector<Obstacle>& getObstacles() { return obstacles; }
    std::vector<Vec3>& getCollectibles() { return collectibles; }
    std::vector<Tree>& getTrees() { return trees; }
    std::vector<Vec3>& getGrassPatches() { return grassPatches; }
    
    float getScore() const { return score; }
    float getDistance() const { return distance; }
    float getPlayerHealth() const { return player.health; }
    float getGameSpeed() const { return gameSpeed; }
    float getDifficulty() const { return difficulty; }
    float getEnvironmentRotation() const { return environmentRotation; }
    float getEnvironmentRotationSpeed() const { return environmentRotationSpeed; }
    
    bool isGameOver() const { return gameOver; }
    bool isPaused() const { return gamePaused; }
    bool isInMenu() const { return inMenu; }
    
    GameState getState() const { return currentState; }
    
    Vec3 getCameraPosition() const { return cameraPosition; }
    Vec3 getCameraTarget() const { return cameraTarget; }
    
    std::string getStateString() const {
        switch(currentState) {
            case MENU: return "MAIN MENU";
            case PLAYING: return "PLAYING";
            case PAUSED: return "PAUSED";
            case GAME_OVER: return "GAME OVER";
            default: return "UNKNOWN";
        }
    }
};

// Mesh generators
Mesh generateSphere(int segments = 16, int rings = 16) {
    Mesh mesh;
    
    for (int i = 0; i <= rings; i++) {
        float phi = 3.14159f * i / rings;
        
        for (int j = 0; j <= segments; j++) {
            float theta = 2.0f * 3.14159f * j / segments;
            
            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);
            
            mesh.vertices.push_back(Vertex(
                Vec3(x, y, z),
                Vec3(x, y, z).normalize(),
                Vec3(1.0f, 1.0f, 1.0f)
            ));
        }
    }
    
    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < segments; j++) {
            int first = i * (segments + 1) + j;
            int second = first + segments + 1;
            
            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);
            
            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }
    
    return mesh;
}

Mesh generateCube() {
    Mesh mesh;
    
    // Define cube vertices (positions, normals, colors)
    // Front face
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, -0.5f, 0.5f), Vec3(0,0,1), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, -0.5f, 0.5f), Vec3(0,0,1), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, 0.5f, 0.5f), Vec3(0,0,1), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, 0.5f, 0.5f), Vec3(0,0,1), Vec3(1,1,1)));
    
    // Back face
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0,0,-1), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, 0.5f, -0.5f), Vec3(0,0,-1), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, 0.5f, -0.5f), Vec3(0,0,-1), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, -0.5f, -0.5f), Vec3(0,0,-1), Vec3(1,1,1)));
    
    // Left face
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, -0.5f, -0.5f), Vec3(-1,0,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, -0.5f, 0.5f), Vec3(-1,0,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, 0.5f, 0.5f), Vec3(-1,0,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, 0.5f, -0.5f), Vec3(-1,0,0), Vec3(1,1,1)));
    
    // Right face
    mesh.vertices.push_back(Vertex(Vec3(0.5f, -0.5f, 0.5f), Vec3(1,0,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, -0.5f, -0.5f), Vec3(1,0,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, 0.5f, -0.5f), Vec3(1,0,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, 0.5f, 0.5f), Vec3(1,0,0), Vec3(1,1,1)));
    
    // Top face
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, 0.5f, 0.5f), Vec3(0,1,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, 0.5f, 0.5f), Vec3(0,1,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, 0.5f, -0.5f), Vec3(0,1,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, 0.5f, -0.5f), Vec3(0,1,0), Vec3(1,1,1)));
    
    // Bottom face
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0,-1,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, -0.5f, -0.5f), Vec3(0,-1,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, -0.5f, 0.5f), Vec3(0,-1,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, -0.5f, 0.5f), Vec3(0,-1,0), Vec3(1,1,1)));
    
    // Define indices for all faces
    unsigned int indices[] = {
        0,1,2, 2,3,0,    // Front
        4,5,6, 6,7,4,    // Back
        8,9,10, 10,11,8, // Left
        12,13,14, 14,15,12, // Right
        16,17,18, 18,19,16, // Top
        20,21,22, 22,23,20  // Bottom
    };
    
    mesh.indices.assign(indices, indices + 36);
    return mesh;
}

Mesh generatePyramid() {
    Mesh mesh;
    
    // Base
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, 0, -0.5f), Vec3(0,-1,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, 0, -0.5f), Vec3(0,-1,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(0.5f, 0, 0.5f), Vec3(0,-1,0), Vec3(1,1,1)));
    mesh.vertices.push_back(Vertex(Vec3(-0.5f, 0, 0.5f), Vec3(0,-1,0), Vec3(1,1,1)));
    
    // Apex
    mesh.vertices.push_back(Vertex(Vec3(0, 1, 0), Vec3(0,1,0), Vec3(1,1,1)));
    
    // Indices for base
    mesh.indices.push_back(0); mesh.indices.push_back(1); mesh.indices.push_back(2);
    mesh.indices.push_back(2); mesh.indices.push_back(3); mesh.indices.push_back(0);
    
    // Indices for sides
    mesh.indices.push_back(0); mesh.indices.push_back(1); mesh.indices.push_back(4);
    mesh.indices.push_back(1); mesh.indices.push_back(2); mesh.indices.push_back(4);
    mesh.indices.push_back(2); mesh.indices.push_back(3); mesh.indices.push_back(4);
    mesh.indices.push_back(3); mesh.indices.push_back(0); mesh.indices.push_back(4);
    
    return mesh;
}

Mesh generateCylinder(int segments = 16) {
    Mesh mesh;
    
    // Create vertices
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * 3.14159f * i / segments;
        float x = cos(theta);
        float z = sin(theta);
        
        // Bottom vertices
        mesh.vertices.push_back(Vertex(
            Vec3(x * 0.5f, -0.5f, z * 0.5f),
            Vec3(x, 0, z),
            Vec3(1,1,1)
        ));
        
        // Top vertices
        mesh.vertices.push_back(Vertex(
            Vec3(x * 0.5f, 0.5f, z * 0.5f),
            Vec3(x, 0, z),
            Vec3(1,1,1)
        ));
    }
    
    // Create indices for sides
    for (int i = 0; i < segments; i++) {
        int base = i * 2;
        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 3);
    }
    
    return mesh;
}

Mesh generateTreeTrunk(int segments = 8) {
    Mesh mesh;
    
    // Create vertices for trunk
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * 3.14159f * i / segments;
        float x = cos(theta);
        float z = sin(theta);
        
        // Bottom vertices
        mesh.vertices.push_back(Vertex(
            Vec3(x * 0.2f, -0.5f, z * 0.2f),
            Vec3(x, 0, z),
            Vec3(0.4f, 0.2f, 0.1f)
        ));
        
        // Top vertices
        mesh.vertices.push_back(Vertex(
            Vec3(x * 0.15f, 0.5f, z * 0.15f),
            Vec3(x, 0, z),
            Vec3(0.4f, 0.2f, 0.1f)
        ));
    }
    
    // Create indices for sides
    for (int i = 0; i < segments; i++) {
        int base = i * 2;
        mesh.indices.push_back(base);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 3);
    }
    
    return mesh;
}

Mesh generateTreeFoliage(int segments = 16) {
    Mesh mesh;
    
    // Create vertices for foliage (sphere-like shape)
    for (int i = 0; i <= segments; i++) {
        float phi = 3.14159f * i / segments;
        
        for (int j = 0; j <= segments; j++) {
            float theta = 2.0f * 3.14159f * j / segments;
            
            float x = sin(phi) * cos(theta) * 0.8f;
            float y = cos(phi) * 0.8f;
            float z = sin(phi) * sin(theta) * 0.8f;
            
            mesh.vertices.push_back(Vertex(
                Vec3(x, y, z),
                Vec3(x, y, z).normalize(),
                Vec3(0.1f, 0.5f, 0.2f)
            ));
        }
    }
    
    for (int i = 0; i < segments; i++) {
        for (int j = 0; j < segments; j++) {
            int first = i * (segments + 1) + j;
            int second = first + segments + 1;
            
            mesh.indices.push_back(first);
            mesh.indices.push_back(second);
            mesh.indices.push_back(first + 1);
            
            mesh.indices.push_back(second);
            mesh.indices.push_back(second + 1);
            mesh.indices.push_back(first + 1);
        }
    }
    
    return mesh;
}

Mesh generateGrassBlade() {
    Mesh mesh;
    
    // Simple grass blade as a tall thin triangle
    mesh.vertices.push_back(Vertex(Vec3(0, 0, 0), Vec3(0, 1, 0), Vec3(0.1f, 0.6f, 0.1f)));
    mesh.vertices.push_back(Vertex(Vec3(0.05f, 0.5f, 0), Vec3(0, 1, 0), Vec3(0.1f, 0.6f, 0.1f)));
    mesh.vertices.push_back(Vertex(Vec3(-0.05f, 0.5f, 0), Vec3(0, 1, 0), Vec3(0.1f, 0.6f, 0.1f)));
    
    mesh.indices.push_back(0);
    mesh.indices.push_back(1);
    mesh.indices.push_back(2);
    
    return mesh;
}

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;
uniform vec3 viewPos;

out vec3 FragPos;
out vec3 Normal;
out vec3 Color;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Color = aColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 Color;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float time;

void main()
{
    // Material properties
    vec3 materialColor = Color;
    
    // Light properties
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    float ambientStrength = 0.4;
    float specularStrength = 0.3;
    
    // Ambient
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine results
    vec3 result = (ambient + diffuse + specular) * materialColor;
    FragColor = vec4(result, 1.0);
}
)";

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
        std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

void setShaderMat4(GLuint shader, const char* name, const Mat4& matrix) {
    GLint loc = glGetUniformLocation(shader, name);
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, matrix.m);
    }
}

void setShaderVec3(GLuint shader, const char* name, const Vec3& vec) {
    GLint loc = glGetUniformLocation(shader, name);
    if (loc != -1) {
        glUniform3f(loc, vec.x, vec.y, vec.z);
    }
}

void setShaderFloat(GLuint shader, const char* name, float value) {
    GLint loc = glGetUniformLocation(shader, name);
    if (loc != -1) {
        glUniform1f(loc, value);
    }
}

int main() {
    std::cout << "Starting Meta Ball Rolling 3D Game..." << std::endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(1600, 900, "Meta Ball Rolling 3D - Enhanced Environment", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Enable OpenGL features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Create shader
    GLuint shaderProgram = createShaderProgram();
    
    // Create game
    Game game;
    
    // Generate meshes
    Mesh sphereMesh = generateSphere();
    Mesh cubeMesh = generateCube();
    Mesh pyramidMesh = generatePyramid();
    Mesh cylinderMesh = generateCylinder();
    Mesh terrainMesh = game.getTerrain().generateMesh();
    Mesh treeTrunkMesh = generateTreeTrunk();
    Mesh treeFoliageMesh = generateTreeFoliage();
    Mesh grassBladeMesh = generateGrassBlade();
    
    // Setup meshes
    sphereMesh.setupBuffers();
    cubeMesh.setupBuffers();
    pyramidMesh.setupBuffers();
    cylinderMesh.setupBuffers();
    terrainMesh.setupBuffers();
    treeTrunkMesh.setupBuffers();
    treeFoliageMesh.setupBuffers();
    grassBladeMesh.setupBuffers();
    
    // Game state
    bool showDebug = false;
    bool wireframe = false;
    Vec3 lightPos(10.0f, 20.0f, 10.0f);
    float time = 0.0f;
    
    std::cout << "\n=== CONTROLS ===" << std::endl;
    std::cout << "WASD/Arrow Keys: Move ball" << std::endl;
    std::cout << "SPACE: Jump" << std::endl;
    std::cout << "ESC: Pause/Resume" << std::endl;
    std::cout << "R: Toggle Environment Rotation" << std::endl;
    std::cout << "F1: Toggle Debug Info" << std::endl;
    std::cout << "F2: Toggle Wireframe" << std::endl;
    std::cout << "================\n" << std::endl;
    
    // Main game loop
    float lastTime = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        time += deltaTime;
        
        // Limit delta time to avoid large jumps
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        // Handle input
        Game::GameState currentGameState = game.getState(); // Get current state
        
        if (currentGameState == Game::PLAYING) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                game.handleInput(GLFW_KEY_W, GLFW_PRESS);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
                game.handleInput(GLFW_KEY_S, GLFW_PRESS);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
                game.handleInput(GLFW_KEY_A, GLFW_PRESS);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
                game.handleInput(GLFW_KEY_D, GLFW_PRESS);
        }
        
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            game.handleInput(GLFW_KEY_SPACE, GLFW_PRESS);
        
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            static double lastPress = 0;
            if (currentTime - lastPress > 0.3) {
                game.togglePause();
                lastPress = currentTime;
            }
        }
        
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            static double lastPress = 0;
            if (currentTime - lastPress > 0.3) {
                game.handleInput(GLFW_KEY_R, GLFW_PRESS);
                lastPress = currentTime;
            }
        }
        
        if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
            static double lastPress = 0;
            if (currentTime - lastPress > 0.3) {
                showDebug = !showDebug;
                lastPress = currentTime;
            }
        }
        
        if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
            static double lastPress = 0;
            if (currentTime - lastPress > 0.3) {
                wireframe = !wireframe;
                lastPress = currentTime;
            }
        }
        
        // Update game
        game.update(deltaTime);
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Get window size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        
        // Clear screen
        glClearColor(0.53f, 0.81f, 0.98f, 1.0f); // Sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set polygon mode
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        
        // Create matrices
        Mat4 projection = Mat4::perspective(60.0f, (float)width / (float)height, 0.1f, 200.0f);
        Mat4 view = Mat4::lookAt(game.getCameraPosition(), game.getCameraTarget(), Vec3(0, 1, 0));
        
        // Use shader
        glUseProgram(shaderProgram);
        setShaderMat4(shaderProgram, "projection", projection);
        setShaderMat4(shaderProgram, "view", view);
        setShaderVec3(shaderProgram, "lightPos", lightPos);
        setShaderVec3(shaderProgram, "viewPos", game.getCameraPosition());
        setShaderFloat(shaderProgram, "time", time);
        
        // Apply environment rotation
        Mat4 envRotation = Mat4::rotateY(game.getEnvironmentRotation());
        
        // Draw terrain with environment rotation
        setShaderMat4(shaderProgram, "model", envRotation);
        terrainMesh.draw();
        
        // Draw player
        MetaBall& player = game.getPlayer();
        if (player.isAlive) {
            Mat4 playerModel = envRotation * player.getModelMatrix();
            setShaderMat4(shaderProgram, "model", playerModel);
            setShaderVec3(shaderProgram, "lightPos", lightPos);
            sphereMesh.draw();
        }
        
        // Draw obstacles with environment rotation
        for (const auto& obs : game.getObstacles()) {
            if (obs.isActive) {
                Mat4 obstacleModel = envRotation * obs.getModelMatrix();
                setShaderMat4(shaderProgram, "model", obstacleModel);
                
                // Set obstacle color
                setShaderVec3(shaderProgram, "lightPos", lightPos);
                
                switch(obs.type) {
                    case 0: cubeMesh.draw(); break;
                    case 1: pyramidMesh.draw(); break;
                    case 2: cylinderMesh.draw(); break;
                }
            }
        }
        
        // Draw collectibles (as small spheres)
        for (const auto& col : game.getCollectibles()) {
            Mat4 collectibleModel = envRotation * Mat4::translate(col.x, col.y, col.z) * Mat4::scale(0.3f, 0.3f, 0.3f);
            setShaderMat4(shaderProgram, "model", collectibleModel);
            setShaderVec3(shaderProgram, "lightPos", lightPos + Vec3(0, 5, 0)); // Different light for glow effect
            sphereMesh.draw();
        }
        
        // Draw trees
        for (const auto& tree : game.getTrees()) {
            // Draw trunk
            Mat4 trunkModel = envRotation * tree.getTrunkModelMatrix();
            setShaderMat4(shaderProgram, "model", trunkModel);
            setShaderVec3(shaderProgram, "lightPos", lightPos);
            treeTrunkMesh.draw();
            
            // Draw foliage
            Mat4 foliageModel = envRotation * tree.getFoliageModelMatrix();
            setShaderMat4(shaderProgram, "model", foliageModel);
            setShaderVec3(shaderProgram, "lightPos", lightPos + Vec3(0, 3, 0));
            treeFoliageMesh.draw();
        }
        
        // Draw grass
        for (const auto& grass : game.getGrassPatches()) {
            // Draw multiple grass blades in a patch
            for (int i = 0; i < 5; i++) {
                float offsetX = (dis(gen) - 0.5f) * 0.3f;
                float offsetZ = (dis(gen) - 0.5f) * 0.3f;
                float rotation = dis(gen) * 360.0f;
                float scale = 0.2f + dis(gen) * 0.3f;
                
                Mat4 grassModel = envRotation * 
                                 Mat4::translate(grass.x + offsetX, grass.y, grass.z + offsetZ) *
                                 Mat4::rotateY(rotation) *
                                 Mat4::scale(scale, scale, scale);
                
                setShaderMat4(shaderProgram, "model", grassModel);
                setShaderVec3(shaderProgram, "lightPos", lightPos);
                grassBladeMesh.draw();
            }
        }
        
        // Draw GUI based on game state
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::SetNextWindowSize(ImVec2(300, 250));
        ImGui::Begin("Meta Ball Rolling 3D - Enhanced", nullptr, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        
        switch(currentGameState) { // Use the local variable
            case Game::MENU: {
                ImGui::TextColored(ImVec4(1,1,0,1), "META BALL ROLLING 3D");
                ImGui::Separator();
                ImGui::Text("Enhanced Environment Edition!");
                ImGui::Spacing();
                ImGui::Text("Features:");
                ImGui::Text("- Rotating Environment (Press R)");
                ImGui::Text("- Trees & Grass");
                ImGui::Text("- Colorful Terrain");
                ImGui::Spacing();
                
                if (ImGui::Button("START GAME", ImVec2(280, 50))) {
                    game.startGame();
                }
                ImGui::Spacing();
                
                if (ImGui::Button("HOW TO PLAY", ImVec2(280, 30))) {
                    // Show instructions
                }
                ImGui::Spacing();
                
                if (ImGui::Button("EXIT", ImVec2(280, 30))) {
                    glfwSetWindowShouldClose(window, true);
                }
                break;
            }
            
            case Game::PLAYING: {
                // Game HUD
                ImGui::TextColored(ImVec4(0,1,0,1), "SCORE: %.0f", game.getScore());
                ImGui::Text("DISTANCE: %.1f m", game.getDistance());
                ImGui::Text("SPEED: %.1f", game.getGameSpeed());
                ImGui::Text("ENVIRONMENT ROTATION: %s", 
                           game.getEnvironmentRotationSpeed() > 0 ? "ON" : "OFF");
                
                // Health bar
                float health = game.getPlayerHealth();
                ImVec4 healthColor;
                if (health > 70) healthColor = ImVec4(0,1,0,1);
                else if (health > 30) healthColor = ImVec4(1,1,0,1);
                else healthColor = ImVec4(1,0,0,1);
                
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
                ImGui::ProgressBar(health / 100.0f, ImVec2(280, 20), "");
                ImGui::PopStyleColor();
                ImGui::Text("HEALTH: %.0f%%", health);
                
                // Controls reminder
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1,1,1,0.7f), "WASD: Move | SPACE: Jump");
                ImGui::TextColored(ImVec4(1,1,1,0.7f), "R: Toggle Rotation | ESC: Pause");
                break;
            }
            
            case Game::PAUSED: {
                ImGui::TextColored(ImVec4(1,1,0,1), "GAME PAUSED");
                ImGui::Separator();
                ImGui::Text("SCORE: %.0f", game.getScore());
                ImGui::Text("DISTANCE: %.1f m", game.getDistance());
                ImGui::Text("ENVIRONMENT ROTATION: %s", 
                           game.getEnvironmentRotationSpeed() > 0 ? "ON" : "OFF");
                ImGui::Spacing();
                
                if (ImGui::Button("RESUME", ImVec2(280, 40))) {
                    game.togglePause();
                }
                ImGui::Spacing();
                
                if (ImGui::Button("MAIN MENU", ImVec2(280, 40))) {
                    game.returnToMenu();
                }
                break;
            }
            
            case Game::GAME_OVER: {
                ImGui::TextColored(ImVec4(1,0,0,1), "GAME OVER!");
                ImGui::Separator();
                ImGui::Text("FINAL SCORE: %.0f", game.getScore());
                ImGui::Text("DISTANCE: %.1f m", game.getDistance());
                ImGui::Spacing();
                
                if (ImGui::Button("PLAY AGAIN", ImVec2(280, 50))) {
                    game.startGame();
                }
                ImGui::Spacing();
                
                if (ImGui::Button("MAIN MENU", ImVec2(280, 40))) {
                    game.returnToMenu();
                }
                break;
            }
        }
        
        ImGui::End();
        
        // Debug window
        if (showDebug) {
            ImGui::Begin("Debug Info", &showDebug);
            ImGui::Text("Game State: %s", game.getStateString().c_str());
            ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
            ImGui::Text("Ball Position: %.2f, %.2f, %.2f", 
                       game.getPlayer().position.x,
                       game.getPlayer().position.y,
                       game.getPlayer().position.z);
            ImGui::Text("Ball Velocity: %.2f, %.2f, %.2f",
                       game.getPlayer().velocity.x,
                       game.getPlayer().velocity.y,
                       game.getPlayer().velocity.z);
            ImGui::Text("Difficulty: %.2f", game.getDifficulty());
            ImGui::Text("Environment Rotation: %.1f°", game.getEnvironmentRotation());
            ImGui::Text("Obstacles: %zu", game.getObstacles().size());
            ImGui::Text("Collectibles: %zu", game.getCollectibles().size());
            ImGui::Text("Trees: %zu", game.getTrees().size());
            ImGui::Text("Grass Patches: %zu", game.getGrassPatches().size());
            ImGui::End();
        }
        
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    sphereMesh.cleanup();
    cubeMesh.cleanup();
    pyramidMesh.cleanup();
    cylinderMesh.cleanup();
    terrainMesh.cleanup();
    treeTrunkMesh.cleanup();
    treeFoliageMesh.cleanup();
    grassBladeMesh.cleanup();
    glDeleteProgram(shaderProgram);
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}