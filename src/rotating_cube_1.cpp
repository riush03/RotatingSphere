#include <iostream>
#include <vector>
#include <cmath>
#include <string>

// OpenGL headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// GLM headers - we'll include them directly if vcpkg installed them
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGui headers
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

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
    LightPos = vec3(view * vec4(lightPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
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
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    // Combine lighting with object color
    vec3 result = (ambient + diffuse + specular) * Color;
    FragColor = vec4(result, 1.0);
}
)";

// Function to generate sphere vertices
void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices,
                   float radius = 1.0f, int sectors = 36, int stacks = 18) {
    
    const float PI = 3.14159265358979323846f;
    
    // Clear vectors
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
            float length = sqrtf(x*x + y*y + z*z);
            vertices.push_back(x / length);
            vertices.push_back(y / length);
            vertices.push_back(z / length);
            
            // Color (gradient based on position)
            vertices.push_back((x + 1.0f) / 2.0f); // Red
            vertices.push_back((y + 1.0f) / 2.0f); // Green
            vertices.push_back((z + 1.0f) / 2.0f); // Blue
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

// Function to set up sphere VAO/VBO/EBO
void setupSphere(GLuint& VAO, GLuint& VBO, GLuint& EBO, 
                const std::vector<float>& vertices, 
                const std::vector<unsigned int>& indices) {
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    // Fill VBO with vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Fill EBO with index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute (3 floats, offset = 3 * sizeof(float))
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Color attribute (3 floats, offset = 6 * sizeof(float))
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

int main() {
    std::cout << "Initializing OpenGL 3D Sphere with ImGui..." << std::endl;
    
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
    generateSphere(sphereVertices, sphereIndices, 1.0f, 32, 16);
    std::cout << "Sphere generated with " << sphereVertices.size()/9 << " vertices and " 
              << sphereIndices.size() << " indices" << std::endl;
    
    // Create sphere VAO, VBO, EBO
    GLuint sphereVAO, sphereVBO, sphereEBO;
    setupSphere(sphereVAO, sphereVBO, sphereEBO, sphereVertices, sphereIndices);
    
    // Get uniform locations
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    
    // Application state
    bool showImGui = true;
    float rotationSpeed = 45.0f;
    float cameraDistance = 3.0f;
    float cameraHeight = 0.5f;
    float cameraAngle = 45.0f;
    glm::vec3 lightPos = glm::vec3(2.0f, 3.0f, 2.0f);
    glm::vec3 backgroundColor = glm::vec3(0.1f, 0.1f, 0.15f);
    bool wireframeMode = false;
    bool rotateSphere = true;
    float sphereRadius = 1.0f;
    int sphereSegments = 32;
    int sphereStacks = 16;
    
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
            // Simple debouncing
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
        
        // Update sphere rotation
        static float rotationAngle = 0.0f;
        if (rotateSphere) {
            rotationAngle += rotationSpeed * deltaTime;
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
            ImGui::SliderFloat("Radius", &sphereRadius, 0.1f, 3.0f);
            
            // Sphere detail
            ImGui::Separator();
            ImGui::Text("Sphere Detail:");
            if (ImGui::SliderInt("Segments", &sphereSegments, 8, 64) ||
                ImGui::SliderInt("Stacks", &sphereStacks, 4, 32)) {
                // Regenerate sphere if detail changed
                generateSphere(sphereVertices, sphereIndices, sphereRadius, sphereSegments, sphereStacks);
                setupSphere(sphereVAO, sphereVBO, sphereEBO, sphereVertices, sphereIndices);
            }
            
            // Lighting controls
            ImGui::Separator();
            ImGui::Text("Lighting:");
            ImGui::SliderFloat3("Light Position", &lightPos[0], -5.0f, 5.0f);
            
            // Display settings
            ImGui::Separator();
            ImGui::Text("Display:");
            ImGui::Checkbox("Wireframe Mode", &wireframeMode);
            ImGui::ColorEdit3("Background", &backgroundColor[0]);
            
            // Reset button
            ImGui::Separator();
            if (ImGui::Button("Reset to Defaults")) {
                rotationSpeed = 45.0f;
                cameraDistance = 3.0f;
                cameraHeight = 0.5f;
                cameraAngle = 45.0f;
                lightPos = glm::vec3(2.0f, 3.0f, 2.0f);
                backgroundColor = glm::vec3(0.1f, 0.1f, 0.15f);
                sphereRadius = 1.0f;
                sphereSegments = 32;
                sphereStacks = 16;
                
                // Regenerate sphere with new radius
                generateSphere(sphereVertices, sphereIndices, sphereRadius, sphereSegments, sphereStacks);
                setupSphere(sphereVAO, sphereVBO, sphereEBO, sphereVertices, sphereIndices);
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
        glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set wireframe mode
        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.5f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        
        // Calculate camera position
        glm::vec3 cameraPos = glm::vec3(
            cameraDistance * cos(glm::radians(cameraAngle)),
            cameraHeight,
            cameraDistance * sin(glm::radians(cameraAngle))
        );
        
        // Create view matrix
        glm::mat4 view = glm::lookAt(
            cameraPos,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        // Create projection matrix
        glm::mat4 projection = glm::perspective(
            glm::radians(60.0f),
            (float)width / (float)height,
            0.1f,
            100.0f
        );
        
        // Create model matrix for sphere
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.5f));
        model = glm::scale(model, glm::vec3(sphereRadius));
        
        // Use shader program
        glUseProgram(shaderProgram);
        
        // Pass matrices to shader
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        
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
    
    // Destroy window and terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "Program exited successfully" << std::endl;
    return 0;
}