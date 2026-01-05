#include <iostream>
#include <vector>
#include <cmath>
#include <string>

// OpenGL headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Check if SDL3 is available
#ifdef HAS_SDL3
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#endif

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
    float specularStrength = 0.8;
    vec3 viewDir = normalize(-FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    // Combine lighting with object color
    vec3 result = (ambient + diffuse + specular) * Color;
    FragColor = vec4(result, 1.0);
}
)";

// Function to generate sphere vertices
void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                   float radius = 1.0f, int sectors = 32, int stacks = 16) {
    
    const float PI = 3.14159265358979323846f;
    
    vertices.clear();
    indices.clear();
    
    // Generate vertices
    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2.0f - i * (PI / stacks);
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);
        
        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * (2.0f * PI / sectors);
            
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            
            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            // Normal (normalized position)
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);
            
            // Color (based on position for nice gradient)
            vertices.push_back((x + radius) / (2.0f * radius)); // Red
            vertices.push_back((y + radius) / (2.0f * radius)); // Green
            vertices.push_back((z + radius) / (2.0f * radius)); // Blue
        }
    }
    
    // Generate indices
    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;
        
        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            
            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

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

int main() {
    std::cout << "Initializing OpenGL Sphere Game..." << std::endl;
    
#ifdef HAS_SDL3
    std::cout << "SDL3 is available!" << std::endl;
    
    // Initialize SDL3 - Use 0 or SDL_INIT_VIDEO only
    SDL_SetMainReady();
    if (SDL_Init(0) < 0) {  // Or use SDL_INIT_VIDEO if defined
        std::cerr << "SDL3 initialization failed: " << SDL_GetError() << std::endl;
    } else {
        std::cout << "SDL3 initialized successfully" << std::endl;
        
        // Try to initialize video separately
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL3 video subsystem failed: " << SDL_GetError() << std::endl;
        } else {
            std::cout << "SDL3 video subsystem initialized" << std::endl;
        }
    }
#endif
    
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
    GLFWwindow* window = glfwCreateWindow(1280, 720, "3D Sphere with ImGui", NULL, NULL);
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
    
    // Generate sphere geometry
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(sphereVertices, sphereIndices);
    std::cout << "Sphere generated with " << sphereVertices.size()/9 << " vertices and " 
              << sphereIndices.size() << " indices" << std::endl;
    
    // Create sphere VAO, VBO, EBO
    GLuint sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    
    glBindVertexArray(sphereVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Color attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    // Application state
    bool showImGui = true;
    float rotationSpeed = 45.0f;
    float cameraDistance = 3.0f;
    float cameraHeight = 0.5f;
    float cameraAngle = 45.0f;
    Vec3 lightPos = Vec3(2.0f, 3.0f, 2.0f);
    float backgroundColor[3] = {0.1f, 0.1f, 0.15f};
    bool wireframeMode = false;
    bool rotateSphere = true;
    float sphereRadius = 1.0f;
    int sphereSegments = 32;
    int sphereStacks = 16;
    bool regenerateSphere = false;
    
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
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // ImGui menu window
        if (showImGui) {
            ImGui::Begin("3D Sphere Controls", &showImGui, ImGuiWindowFlags_AlwaysAutoResize);
            
            // Performance info
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);
            ImGui::Separator();
            
            // Camera controls
            ImGui::Text("Camera Settings:");
            ImGui::SliderFloat("Distance", &cameraDistance, 1.0f, 10.0f);
            ImGui::SliderFloat("Height", &cameraHeight, -2.0f, 2.0f);
            ImGui::SliderFloat("Angle", &cameraAngle, 0.0f, 360.0f);
            
            // Sphere controls
            ImGui::Separator();
            ImGui::Text("Sphere Settings:");
            ImGui::Checkbox("Auto Rotate", &rotateSphere);
            ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 180.0f);
            
            if (ImGui::SliderFloat("Radius", &sphereRadius, 0.1f, 3.0f)) {
                regenerateSphere = true;
            }
            
            // Sphere detail
            ImGui::Separator();
            ImGui::Text("Sphere Detail:");
            if (ImGui::SliderInt("Segments", &sphereSegments, 8, 64)) {
                regenerateSphere = true;
            }
            if (ImGui::SliderInt("Stacks", &sphereStacks, 4, 32)) {
                regenerateSphere = true;
            }
            
            // Lighting controls
            ImGui::Separator();
            ImGui::Text("Lighting:");
            ImGui::SliderFloat3("Light Position", &lightPos.x, -5.0f, 5.0f);
            
            // Display settings
            ImGui::Separator();
            ImGui::Text("Display:");
            ImGui::Checkbox("Wireframe Mode", &wireframeMode);
            ImGui::ColorEdit3("Background", backgroundColor);
            
            // Regenerate sphere if needed
            if (regenerateSphere) {
                generateSphere(sphereVertices, sphereIndices, sphereRadius, sphereSegments, sphereStacks);
                glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
                glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
                regenerateSphere = false;
            }
            
            // Reset button
            ImGui::Separator();
            if (ImGui::Button("Reset to Defaults")) {
                rotationSpeed = 45.0f;
                cameraDistance = 3.0f;
                cameraHeight = 0.5f;
                cameraAngle = 45.0f;
                lightPos = Vec3(2.0f, 3.0f, 2.0f);
                backgroundColor[0] = 0.1f;
                backgroundColor[1] = 0.1f;
                backgroundColor[2] = 0.15f;
                sphereRadius = 1.0f;
                sphereSegments = 32;
                sphereStacks = 16;
                regenerateSphere = true;
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
        Mat4 view = Mat4::lookAt(cameraPos, Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
        
        // Create model matrix for sphere
        static float rotationAngle = 0.0f;
        if (rotateSphere) {
            rotationAngle += rotationSpeed * deltaTime * 3.14159f / 180.0f;
        }
        
        // Fixed: Use matrix multiplication with the operator we defined
        Mat4 rotationY = Mat4::rotateY(rotationAngle);
        Mat4 rotationX = Mat4::rotateX(rotationAngle * 0.5f);
        Mat4 model = rotationY * rotationX;
        
        // Use shader program
        glUseProgram(shaderProgram);
        
        // Pass matrices to shader
        setShaderMat4(shaderProgram, "model", model);
        setShaderMat4(shaderProgram, "view", view);
        setShaderMat4(shaderProgram, "projection", projection);
        setShaderVec3(shaderProgram, "lightPos", lightPos);
        
        // Draw sphere
        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
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
    
    // Cleanup OpenGL resources
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteProgram(shaderProgram);
    
    // Cleanup SDL3
#ifdef HAS_SDL3
    SDL_Quit();
#endif
    
    // Destroy window and terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "Program exited successfully" << std::endl;
    return 0;
}