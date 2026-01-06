// src/main.cpp - Fixed Viewport Rendering

// GLM experimental features must be enabled before including headers
#define GLM_ENABLE_EXPERIMENTAL

// Include guards for Windows
#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <windows.h>
    #include <direct.h>
    #define GETCWD _getcwd
    #define PATH_SEPARATOR "\\"
    #define strcasecmp _stricmp
    #include <commdlg.h>
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #define GETCWD getcwd
    #define PATH_SEPARATOR "/"
#endif

// OpenGL headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
    #include <GLFW/glfw3native.h>
#endif

// Standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <string>
#include <float.h>
#include <algorithm>
#include <memory>
#include <map>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// GLM (GLM_ENABLE_EXPERIMENTAL must be defined before including)
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// tinyobjloader
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

// Simple GLB loader
struct GLBModel {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 bboxMin, bboxMax;
    std::string name;
};

// Color scheme
#define COLOR_BG ImVec4(0.08f, 0.08f, 0.12f, 1.00f)
#define COLOR_WINDOW_BG ImVec4(0.12f, 0.12f, 0.16f, 1.00f)
#define COLOR_HEADER ImVec4(0.18f, 0.18f, 0.24f, 1.00f)
#define COLOR_BUTTON ImVec4(0.26f, 0.59f, 0.98f, 0.60f)
#define COLOR_BUTTON_HOVER ImVec4(0.26f, 0.59f, 0.98f, 0.80f)
#define COLOR_BUTTON_ACTIVE ImVec4(0.06f, 0.53f, 0.98f, 1.00f)
#define COLOR_ACCENT ImVec4(0.98f, 0.39f, 0.36f, 1.00f)
#define COLOR_TEXT ImVec4(0.95f, 0.96f, 0.98f, 1.00f)
#define COLOR_TEXT_DIM ImVec4(0.70f, 0.70f, 0.75f, 1.00f)
#define COLOR_SUCCESS ImVec4(0.2f, 0.8f, 0.4f, 1.00f)
#define COLOR_WARNING ImVec4(1.0f, 0.8f, 0.2f, 1.00f)

// 3D Object structure with transform controls
struct GameObject {
    GLuint vao, vbo, ebo;
    int vertexCount;
    int indexCount;
    glm::vec3 bboxMin, bboxMax;
    glm::vec3 position;
    glm::vec3 rotation; // Euler angles in radians
    glm::vec3 scale;
    glm::vec3 color;
    char name[256];
    bool visible;
    bool selected;
    
    GameObject() : position(0.0f), rotation(0.0f), scale(1.0f), 
                   color(0.8f, 0.8f, 0.8f), visible(true), selected(false),
                   vao(0), vbo(0), ebo(0), vertexCount(0), indexCount(0) {
        bboxMin = glm::vec3(-0.5f);
        bboxMax = glm::vec3(0.5f);
        strcpy_s(name, "Unnamed Object");
    }
    
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};

// File entry structure
struct FileEntry {
    char name[256];
    char path[1024];
    bool isDirectory;
    time_t modifiedTime;
    size_t size;
};

// Transform Gizmo mode
enum TransformMode {
    TRANSFORM_TRANSLATE,
    TRANSFORM_ROTATE,
    TRANSFORM_SCALE
};

// Global variables
GLFWwindow* window;
int windowWidth = 1920;
int windowHeight = 1080;
std::vector<std::shared_ptr<GameObject>> objects;
int selectedObjectIndex = -1;
TransformMode transformMode = TRANSFORM_TRANSLATE;
float cameraDistance = 10.0f;
float cameraYaw = 0.0f;
float cameraPitch = 0.3f;
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 lightPos = glm::vec3(5.0f, 5.0f, 5.0f);
float backgroundColor[4] = {0.08f, 0.08f, 0.12f, 1.0f};
float lightColor[3] = {1.0f, 1.0f, 1.0f};
char currentDirectory[1024] = ".";
std::vector<FileEntry> fileEntries;
char statusMessage[256] = "Ready";
bool wireframeMode = false;
bool showGrid = true;
bool showAxes = true;
bool showBoundingBoxes = false;
float gridSize = 20.0f;
float gizmoSize = 1.0f;
bool snapToGrid = false;
float gridSnapSize = 1.0f;
bool showStatsWindow = true;
bool showTransformWindow = true;
bool showLightWindow = true;
bool showObjectListWindow = true;

// Viewport rendering variables
static ImVec2 viewportPos(0, 0);
static ImVec2 viewportSize(0, 0);
static GLuint viewportFramebuffer = 0;
static GLuint viewportTexture = 0;

// Mouse state variables
static glm::vec2 lastMousePos(0.0f, 0.0f);
static bool isMouseDragging = false;
static int dragButton = -1;
static bool isViewportHovered = false;
static bool isViewportFocused = false;

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
out vec3 FragPos;
out vec3 Normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform int useUniformColor;
void main() {
    vec3 color = objectColor;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 ambient = 0.2 * lightColor;
    vec3 result = (ambient + diffuse) * color;
    FragColor = vec4(result, 1.0);
}
)";

const char* gridVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 Color;
uniform mat4 view;
uniform mat4 projection;
void main() {
    Color = aColor;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

const char* gridFragmentShader = R"(
#version 330 core
in vec3 Color;
out vec4 FragColor;
void main() {
    FragColor = vec4(Color, 1.0);
}
)";

const char* gizmoVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 Color;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;
void main() {
    Color = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* gizmoFragmentShader = R"(
#version 330 core
in vec3 Color;
out vec4 FragColor;
void main() {
    FragColor = vec4(Color, 1.0);
}
)";

// Function prototypes
void initGLFW();
void initGLEW();
void initImGui();
void setupStyle();
GLuint compileShader(const char* source, GLenum type);
GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource);
void createPrimitive(const char* type, const glm::vec3& color = glm::vec3(0.8f, 0.8f, 0.8f));
void createCube();
void createSphere(int segments = 32);
void createCylinder(int segments = 32);
void createCone(int segments = 32);
void createPlane();
void loadOBJModel(const char* path);
bool loadGLBModel(const char* path);
void loadFileList();
bool isModelFile(const char* filename);
void renderObject(const GameObject& obj, GLuint shaderProgram);
void renderGrid(GLuint shaderProgram);
void renderAxes(GLuint shaderProgram);
void renderGizmo(GLuint shaderProgram);
void updateCamera();
void showMainMenuBar();
void showStatusBar();
void showLeftPanel();
void showRightPanel();
void showViewport();
void centerAllModels();
void autoCenterSelectedModel();
void render3DSceneToViewport();
void createViewportFramebuffer();
void resizeViewportFramebuffer(int width, int height);
std::string formatFileSize(size_t size);
std::string formatTime(time_t time);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void drop_callback(GLFWwindow* window, int count, const char** paths);
std::string openFileDialog(const char* filter);

// Shader programs
GLuint modelShader = 0;
GLuint gridShader = 0;
GLuint gizmoShader = 0;

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    
    initGLFW();
    
    // Initialize GLEW
    initGLEW();
    
    // Initialize ImGui
    initImGui();
    
    // Setup custom style
    setupStyle();
    
    // Compile shaders
    modelShader = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    gridShader = createShaderProgram(gridVertexShader, gridFragmentShader);
    gizmoShader = createShaderProgram(gizmoVertexShader, gizmoFragmentShader);
    
    // Create default objects
    createPrimitive("Cube", glm::vec3(0.8f, 0.4f, 0.4f));
    createPrimitive("Sphere", glm::vec3(0.4f, 0.8f, 0.4f));
    createPrimitive("Cylinder", glm::vec3(0.4f, 0.4f, 0.8f));
    
    // Center all models
    centerAllModels();
    
    // Load initial file list
    GETCWD(currentDirectory, sizeof(currentDirectory));
    loadFileList();
    
    // Create viewport framebuffer
    createViewportFramebuffer();
    
    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetDropCallback(window, drop_callback);
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Show main menu
        showMainMenuBar();
        
        // Calculate layout
        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float statusBarHeight = 20.0f;
        float leftPanelWidth = 280.0f;
        float rightPanelWidth = 280.0f;
        float margin = 10.0f;
        
        // Calculate viewport position and size (centered)
        viewportPos = ImVec2(margin + leftPanelWidth, menuBarHeight + margin);
        viewportSize = ImVec2(
            mainViewport->Size.x - (leftPanelWidth + rightPanelWidth + 3 * margin),
            mainViewport->Size.y - (menuBarHeight + statusBarHeight + 2 * margin)
        );
        
        // Render 3D scene to framebuffer
        render3DSceneToViewport();
        
        // Show panels (arranged around the viewport)
        showLeftPanel();
        showRightPanel();
        showViewport();
        
        // Show status bar
        showStatusBar();
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    for (auto& obj : objects) {
        if (obj->vao) glDeleteVertexArrays(1, &obj->vao);
        if (obj->vbo) glDeleteBuffers(1, &obj->vbo);
        if (obj->ebo) glDeleteBuffers(1, &obj->ebo);
    }
    
    if (viewportFramebuffer) glDeleteFramebuffers(1, &viewportFramebuffer);
    if (viewportTexture) glDeleteTextures(1, &viewportTexture);
    
    if (modelShader) glDeleteProgram(modelShader);
    if (gridShader) glDeleteProgram(gridShader);
    if (gizmoShader) glDeleteProgram(gizmoShader);
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}

void initGLFW() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 8); // MSAA
    
    window = glfwCreateWindow(windowWidth, windowHeight, "3D Model Viewer Pro", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        exit(-1);
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
}

void initGLEW() {
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        glfwTerminate();
        exit(-1);
    }
}

void initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Colors
    style.Colors[ImGuiCol_Text] = COLOR_TEXT;
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = COLOR_WINDOW_BG;
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.94f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.32f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = COLOR_HEADER;
    style.Colors[ImGuiCol_TitleBgActive] = COLOR_HEADER;
    style.Colors[ImGuiCol_TitleBgCollapsed] = COLOR_HEADER;
    style.Colors[ImGuiCol_MenuBarBg] = COLOR_HEADER;
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = COLOR_ACCENT;
    style.Colors[ImGuiCol_SliderGrab] = COLOR_BUTTON;
    style.Colors[ImGuiCol_SliderGrabActive] = COLOR_BUTTON_ACTIVE;
    style.Colors[ImGuiCol_Button] = COLOR_BUTTON;
    style.Colors[ImGuiCol_ButtonHovered] = COLOR_BUTTON_HOVER;
    style.Colors[ImGuiCol_ButtonActive] = COLOR_BUTTON_ACTIVE;
    style.Colors[ImGuiCol_Header] = COLOR_HEADER;
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.26f, 0.32f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_Tab] = COLOR_HEADER;
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.26f, 0.32f, 1.00f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused] = COLOR_HEADER;
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = COLOR_ACCENT;
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = COLOR_ACCENT;
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TableHeaderBg] = COLOR_HEADER;
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    
    // Style
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(6, 4);
    style.CellPadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 4);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.IndentSpacing = 20;
    style.ScrollbarSize = 16;
    style.GrabMinSize = 12;
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 0;
    style.TabBorderSize = 0;
    style.WindowRounding = 8;
    style.ChildRounding = 8;
    style.FrameRounding = 4;
    style.PopupRounding = 8;
    style.ScrollbarRounding = 8;
    style.GrabRounding = 4;
    style.TabRounding = 4;
    style.WindowMenuButtonPosition = ImGuiDir_Left;
}

GLuint compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader compilation failed:\n%s\n", infoLog);
    }
    
    return shader;
}

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Shader program linking failed:\n%s\n", infoLog);
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

void createPrimitive(const char* type, const glm::vec3& color) {
    auto obj = std::make_shared<GameObject>();
    obj->color = color;
    
    if (strcmp(type, "Cube") == 0) {
        createCube();
    } else if (strcmp(type, "Sphere") == 0) {
        createSphere();
    } else if (strcmp(type, "Cylinder") == 0) {
        createCylinder();
    } else if (strcmp(type, "Cone") == 0) {
        createCone();
    } else if (strcmp(type, "Plane") == 0) {
        createPlane();
    } else {
        createCube(); // Default to cube
    }
    
    // Copy the last created object's buffers
    if (!objects.empty()) {
        obj->vao = objects.back()->vao;
        obj->vbo = objects.back()->vbo;
        obj->ebo = objects.back()->ebo;
        obj->vertexCount = objects.back()->vertexCount;
        obj->indexCount = objects.back()->indexCount;
        obj->bboxMin = objects.back()->bboxMin;
        obj->bboxMax = objects.back()->bboxMax;
    }
    
    snprintf(obj->name, sizeof(obj->name), "%s %zu", type, objects.size() + 1);
    objects.push_back(obj);
    selectedObjectIndex = objects.size() - 1;
    
    // Auto-center the new model
    autoCenterSelectedModel();
}

void createCube() {
    auto obj = std::make_shared<GameObject>();
    
    float vertices[] = {
        // positions          // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };
    
    glGenVertexArrays(1, &obj->vao);
    glGenBuffers(1, &obj->vbo);
    glGenBuffers(1, &obj->ebo);
    
    glBindVertexArray(obj->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    obj->vertexCount = 24;
    obj->indexCount = 36;
    obj->bboxMin = glm::vec3(-0.5f, -0.5f, -0.5f);
    obj->bboxMax = glm::vec3(0.5f, 0.5f, 0.5f);
    
    objects.push_back(obj);
}

void createSphere(int segments) {
    auto obj = std::make_shared<GameObject>();
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    for (int y = 0; y <= segments; ++y) {
        for (int x = 0; x <= segments; ++x) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)segments;
            
            float xPos = cos(xSegment * 2.0f * glm::pi<float>()) * sin(ySegment * glm::pi<float>());
            float yPos = cos(ySegment * glm::pi<float>());
            float zPos = sin(xSegment * 2.0f * glm::pi<float>()) * sin(ySegment * glm::pi<float>());
            
            vertices.push_back(xPos * 0.5f);
            vertices.push_back(yPos * 0.5f);
            vertices.push_back(zPos * 0.5f);
            
            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
        }
    }
    
    for (int y = 0; y < segments; ++y) {
        for (int x = 0; x < segments; ++x) {
            int first = (y * (segments + 1)) + x;
            int second = first + segments + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    glGenVertexArrays(1, &obj->vao);
    glGenBuffers(1, &obj->vbo);
    glGenBuffers(1, &obj->ebo);
    
    glBindVertexArray(obj->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    obj->vertexCount = static_cast<int>(vertices.size() / 6);
    obj->indexCount = static_cast<int>(indices.size());
    obj->bboxMin = glm::vec3(-0.5f, -0.5f, -0.5f);
    obj->bboxMax = glm::vec3(0.5f, 0.5f, 0.5f);
    
    objects.push_back(obj);
}

void createCylinder(int segments) {
    auto obj = std::make_shared<GameObject>();
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Top circle
    vertices.push_back(0.0f); vertices.push_back(0.5f); vertices.push_back(0.0f);
    vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
    
    // Bottom circle
    vertices.push_back(0.0f); vertices.push_back(-0.5f); vertices.push_back(0.0f);
    vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
    
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * 2.0f * glm::pi<float>();
        float x = cos(angle) * 0.5f;
        float z = sin(angle) * 0.5f;
        
        // Top vertex
        vertices.push_back(x); vertices.push_back(0.5f); vertices.push_back(z);
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
        
        // Bottom vertex
        vertices.push_back(x); vertices.push_back(-0.5f); vertices.push_back(z);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        
        // Side vertices
        vertices.push_back(x); vertices.push_back(0.5f); vertices.push_back(z);
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(z);
        
        vertices.push_back(x); vertices.push_back(-0.5f); vertices.push_back(z);
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(z);
    }
    
    // Top face indices
    for (int i = 0; i < segments; ++i) {
        indices.push_back(0);
        indices.push_back(2 + i * 6);
        indices.push_back(2 + ((i + 1) % segments) * 6);
    }
    
    // Bottom face indices
    for (int i = 0; i < segments; ++i) {
        indices.push_back(1);
        indices.push_back(2 + ((i + 1) % segments) * 6 + 2);
        indices.push_back(2 + i * 6 + 2);
    }
    
    // Side indices
    for (int i = 0; i < segments; ++i) {
        int topLeft = 2 + i * 6 + 4;
        int topRight = 2 + ((i + 1) % segments) * 6 + 4;
        int bottomLeft = 2 + i * 6 + 5;
        int bottomRight = 2 + ((i + 1) % segments) * 6 + 5;
        
        indices.push_back(topLeft);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);
        
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
    }
    
    glGenVertexArrays(1, &obj->vao);
    glGenBuffers(1, &obj->vbo);
    glGenBuffers(1, &obj->ebo);
    
    glBindVertexArray(obj->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    obj->vertexCount = static_cast<int>(vertices.size() / 6);
    obj->indexCount = static_cast<int>(indices.size());
    obj->bboxMin = glm::vec3(-0.5f, -0.5f, -0.5f);
    obj->bboxMax = glm::vec3(0.5f, 0.5f, 0.5f);
    
    objects.push_back(obj);
}

void createCone(int segments) {
    auto obj = std::make_shared<GameObject>();
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Top point
    vertices.push_back(0.0f); vertices.push_back(0.5f); vertices.push_back(0.0f);
    vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
    
    // Bottom circle center
    vertices.push_back(0.0f); vertices.push_back(-0.5f); vertices.push_back(0.0f);
    vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
    
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * 2.0f * glm::pi<float>();
        float x = cos(angle) * 0.5f;
        float z = sin(angle) * 0.5f;
        
        // Bottom circle vertex
        vertices.push_back(x); vertices.push_back(-0.5f); vertices.push_back(z);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        
        // Side vertex for normal calculation
        float nx = cos(angle);
        float nz = sin(angle);
        float ny = 0.5f / sqrtf(0.5f * 0.5f + 1.0f);
        
        vertices.push_back(x); vertices.push_back(-0.5f); vertices.push_back(z);
        vertices.push_back(nx); vertices.push_back(ny); vertices.push_back(nz);
        
        vertices.push_back(0.0f); vertices.push_back(0.5f); vertices.push_back(0.0f);
        vertices.push_back(nx); vertices.push_back(ny); vertices.push_back(nz);
    }
    
    // Bottom face indices
    for (int i = 0; i < segments; ++i) {
        indices.push_back(1);
        indices.push_back(2 + ((i + 1) % segments) * 3);
        indices.push_back(2 + i * 3);
    }
    
    // Side face indices
    for (int i = 0; i < segments; ++i) {
        indices.push_back(2 + i * 3 + 1);
        indices.push_back(2 + ((i + 1) % segments) * 3 + 1);
        indices.push_back(2 + i * 3 + 2);
    }
    
    glGenVertexArrays(1, &obj->vao);
    glGenBuffers(1, &obj->vbo);
    glGenBuffers(1, &obj->ebo);
    
    glBindVertexArray(obj->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    obj->vertexCount = static_cast<int>(vertices.size() / 6);
    obj->indexCount = static_cast<int>(indices.size());
    obj->bboxMin = glm::vec3(-0.5f, -0.5f, -0.5f);
    obj->bboxMax = glm::vec3(0.5f, 0.5f, 0.5f);
    
    objects.push_back(obj);
}

void createPlane() {
    auto obj = std::make_shared<GameObject>();
    
    float vertices[] = {
        // positions          // normals
        -1.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         1.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         1.0f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
        -1.0f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0
    };
    
    glGenVertexArrays(1, &obj->vao);
    glGenBuffers(1, &obj->vbo);
    glGenBuffers(1, &obj->ebo);
    
    glBindVertexArray(obj->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    obj->vertexCount = 4;
    obj->indexCount = 6;
    obj->bboxMin = glm::vec3(-1.0f, 0.0f, -1.0f);
    obj->bboxMax = glm::vec3(1.0f, 0.0f, 1.0f);
    
    objects.push_back(obj);
}

void loadOBJModel(const char* path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
        snprintf(statusMessage, sizeof(statusMessage), "Failed to load OBJ: %s", err.c_str());
        return;
    }
    
    if (!warn.empty()) {
        snprintf(statusMessage, sizeof(statusMessage), "OBJ warning: %s", warn.c_str());
    }
    
    auto obj = std::make_shared<GameObject>();
    
    // Combine all shapes into single vertex array
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    glm::vec3 bboxMin = glm::vec3(FLT_MAX);
    glm::vec3 bboxMax = glm::vec3(-FLT_MAX);
    
    unsigned int indexOffset = 0;
    
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            // Positions
            float px = attrib.vertices[3 * index.vertex_index + 0];
            float py = attrib.vertices[3 * index.vertex_index + 1];
            float pz = attrib.vertices[3 * index.vertex_index + 2];
            
            // Update bounding box
            bboxMin.x = fmin(bboxMin.x, px);
            bboxMin.y = fmin(bboxMin.y, py);
            bboxMin.z = fmin(bboxMin.z, pz);
            bboxMax.x = fmax(bboxMax.x, px);
            bboxMax.y = fmax(bboxMax.y, py);
            bboxMax.z = fmax(bboxMax.z, pz);
            
            // Normals
            float nx = 0.0f, ny = 0.0f, nz = 0.0f;
            if (index.normal_index >= 0) {
                nx = attrib.normals[3 * index.normal_index + 0];
                ny = attrib.normals[3 * index.normal_index + 1];
                nz = attrib.normals[3 * index.normal_index + 2];
            } else {
                // Calculate normal from face if not provided
                // For simplicity, we'll use a default up normal
                nx = 0.0f; ny = 1.0f; nz = 0.0f;
            }
            
            vertices.push_back(px);
            vertices.push_back(py);
            vertices.push_back(pz);
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
            
            indices.push_back(indexOffset++);
        }
    }
    
    if (vertices.empty()) {
        snprintf(statusMessage, sizeof(statusMessage), "No vertices found in OBJ file");
        return;
    }
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &obj->vao);
    glGenBuffers(1, &obj->vbo);
    glGenBuffers(1, &obj->ebo);
    
    glBindVertexArray(obj->vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    // Set object properties
    obj->vertexCount = static_cast<int>(vertices.size() / 6);
    obj->indexCount = static_cast<int>(indices.size());
    obj->bboxMin = bboxMin;
    obj->bboxMax = bboxMax;
    
    // Extract filename from path
    const char* filename = strrchr(path, '/');
    if (!filename) filename = strrchr(path, '\\');
    if (filename) filename++;
    else filename = path;
    
    strncpy_s(obj->name, sizeof(obj->name), filename, sizeof(obj->name) - 1);
    obj->name[sizeof(obj->name) - 1] = '\0';
    
    // Remove extension
    char* dot = strrchr(obj->name, '.');
    if (dot) *dot = '\0';
    
    // Calculate center and offset to position at origin
    glm::vec3 center = (bboxMin + bboxMax) * 0.5f;
    obj->position = -center;
    
    // Scale the object to fit nicely in view
    glm::vec3 size = bboxMax - bboxMin;
    float maxSize = glm::max(glm::max(size.x, size.y), size.z);
    if (maxSize > 0.0f) {
        float scaleFactor = 2.0f / maxSize;
        obj->scale = glm::vec3(scaleFactor);
    }
    
    objects.push_back(obj);
    selectedObjectIndex = objects.size() - 1;
    
    snprintf(statusMessage, sizeof(statusMessage), "Loaded OBJ: %s (%d vertices)", 
             obj->name, obj->vertexCount);
    
    // Auto-center the loaded model
    autoCenterSelectedModel();
}

bool loadGLBModel(const char* path) {
    // For now, create a simple placeholder
    snprintf(statusMessage, sizeof(statusMessage), 
             "GLB loading not implemented. Creating placeholder cube.");
    
    createPrimitive("Cube", glm::vec3(0.8f, 0.6f, 0.2f));
    strcpy_s(objects.back()->name, "GLB_Placeholder");
    
    return false; // Not implemented
}

void centerAllModels() {
    if (objects.empty()) return;
    
    // Calculate collective bounding box
    glm::vec3 bboxMin = glm::vec3(FLT_MAX);
    glm::vec3 bboxMax = glm::vec3(-FLT_MAX);
    
    for (const auto& obj : objects) {
        glm::vec3 objMin = obj->bboxMin + obj->position;
        glm::vec3 objMax = obj->bboxMax + obj->position;
        
        bboxMin.x = glm::min(bboxMin.x, objMin.x);
        bboxMin.y = glm::min(bboxMin.y, objMin.y);
        bboxMin.z = glm::min(bboxMin.z, objMin.z);
        
        bboxMax.x = glm::max(bboxMax.x, objMax.x);
        bboxMax.y = glm::max(bboxMax.y, objMax.y);
        bboxMax.z = glm::max(bboxMax.z, objMax.z);
    }
    
    // Calculate center
    glm::vec3 center = (bboxMin + bboxMax) * 0.5f;
    
    // Move camera target to center
    cameraTarget = center;
    
    // Adjust camera distance based on bounding box size
    glm::vec3 size = bboxMax - bboxMin;
    float maxSize = glm::max(glm::max(size.x, size.y), size.z);
    cameraDistance = glm::max(5.0f, maxSize * 2.0f);
}

void autoCenterSelectedModel() {
    if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
        auto& obj = objects[selectedObjectIndex];
        
        // Move camera to look at the object
        cameraTarget = obj->position;
        
        // Calculate appropriate camera distance based on object size
        glm::vec3 size = obj->bboxMax - obj->bboxMin;
        float maxSize = glm::max(glm::max(size.x, size.y), size.z);
        cameraDistance = glm::max(3.0f, maxSize * 2.0f);
        
        snprintf(statusMessage, sizeof(statusMessage), "Centered on: %s", obj->name);
    }
}

void createViewportFramebuffer() {
    // Create framebuffer
    glGenFramebuffers(1, &viewportFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, viewportFramebuffer);
    
    // Create texture for framebuffer
    glGenTextures(1, &viewportTexture);
    glBindTexture(GL_TEXTURE_2D, viewportTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, viewportTexture, 0);
    
    // Create renderbuffer for depth and stencil
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer is not complete!\n");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void resizeViewportFramebuffer(int width, int height) {
    if (width <= 0 || height <= 0) return;
    
    glBindTexture(GL_TEXTURE_2D, viewportTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    
    // Note: In a complete implementation, you'd also resize the renderbuffer
}

void render3DSceneToViewport() {
    if (viewportSize.x <= 0 || viewportSize.y <= 0) return;
    
    // Bind the viewport framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, viewportFramebuffer);
    
    // Resize framebuffer if needed
    static int lastViewportWidth = 0, lastViewportHeight = 0;
    int viewportWidth = static_cast<int>(viewportSize.x);
    int viewportHeight = static_cast<int>(viewportSize.y);
    
    if (lastViewportWidth != viewportWidth || lastViewportHeight != viewportHeight) {
        resizeViewportFramebuffer(viewportWidth, viewportHeight);
        lastViewportWidth = viewportWidth;
        lastViewportHeight = viewportHeight;
    }
    
    // Set viewport size
    glViewport(0, 0, viewportWidth, viewportHeight);
    
    // Clear the framebuffer
    glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Set polygon mode for wireframe
    glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);
    
    // Update camera
    updateCamera();
    
    // Get view and projection matrices
    glm::vec3 cameraPos = cameraTarget + glm::vec3(
        sin(cameraYaw) * cos(cameraPitch) * cameraDistance,
        sin(cameraPitch) * cameraDistance,
        cos(cameraYaw) * cos(cameraPitch) * cameraDistance
    );
    
    glm::mat4 view = glm::lookAt(
        cameraPos,
        cameraTarget,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        viewportSize.x / viewportSize.y,
        0.1f, 100.0f
    );
    
    // Render grid
    if (showGrid) {
        glUseProgram(gridShader);
        glUniformMatrix4fv(glGetUniformLocation(gridShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(gridShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        renderGrid(gridShader);
    }
    
    // Render axes
    if (showAxes) {
        renderAxes(gridShader);
    }
    
    // Render all objects
    glUseProgram(modelShader);
    glUniformMatrix4fv(glGetUniformLocation(modelShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(modelShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(modelShader, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(modelShader, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(glGetUniformLocation(modelShader, "lightColor"), lightColor[0], lightColor[1], lightColor[2]);
    glUniform1i(glGetUniformLocation(modelShader, "useUniformColor"), 1);
    
    for (const auto& obj : objects) {
        if (obj->visible) {
            renderObject(*obj, modelShader);
        }
    }
    
    // Render transform gizmo for selected object
    if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
        auto& obj = objects[selectedObjectIndex];
        if (obj->visible) {
            glUseProgram(gizmoShader);
            glUniformMatrix4fv(glGetUniformLocation(gizmoShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(gizmoShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            renderGizmo(gizmoShader);
        }
    }
    
    // Reset polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void loadFileList() {
    fileEntries.clear();
    
    #ifdef _WIN32
        WIN32_FIND_DATAA findData;
        HANDLE hFind;
        char searchPath[1024];
        snprintf(searchPath, sizeof(searchPath), "%s\\*", currentDirectory);
        
        hFind = FindFirstFileA(searchPath, &findData);
        if (hFind == INVALID_HANDLE_VALUE) {
            snprintf(statusMessage, sizeof(statusMessage), "Failed to read directory");
            return;
        }
        
        do {
            if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
                continue;
            
            FileEntry entry;
            strncpy_s(entry.name, sizeof(entry.name), findData.cFileName, sizeof(entry.name) - 1);
            entry.name[sizeof(entry.name) - 1] = '\0';
            
            snprintf(entry.path, sizeof(entry.path), "%s\\%s", currentDirectory, findData.cFileName);
            
            entry.isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            
            // Convert FILETIME to time_t
            ULARGE_INTEGER uli;
            uli.LowPart = findData.ftLastWriteTime.dwLowDateTime;
            uli.HighPart = findData.ftLastWriteTime.dwHighDateTime;
            entry.modifiedTime = (uli.QuadPart - 116444736000000000ULL) / 10000000ULL;
            
            entry.size = ((ULARGE_INTEGER*)&findData.nFileSizeLow)->QuadPart;
            
            fileEntries.push_back(entry);
            
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
    #else
        DIR* dir = opendir(currentDirectory);
        if (!dir) {
            snprintf(statusMessage, sizeof(statusMessage), "Failed to read directory");
            return;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            
            FileEntry fileEntry;
            strncpy(fileEntry.name, entry->d_name, sizeof(fileEntry.name) - 1);
            fileEntry.name[sizeof(fileEntry.name) - 1] = '\0';
            
            snprintf(fileEntry.path, sizeof(fileEntry.path), "%s/%s", currentDirectory, entry->d_name);
            
            // Get file info
            struct stat fileStat;
            if (stat(fileEntry.path, &fileStat) == 0) {
                fileEntry.isDirectory = S_ISDIR(fileStat.st_mode);
                fileEntry.modifiedTime = fileStat.st_mtime;
                fileEntry.size = fileStat.st_size;
            } else {
                fileEntry.isDirectory = false;
                fileEntry.modifiedTime = 0;
                fileEntry.size = 0;
            }
            
            fileEntries.push_back(fileEntry);
        }
        
        closedir(dir);
    #endif
    
    snprintf(statusMessage, sizeof(statusMessage), "Loaded %zu files", fileEntries.size());
}

void renderObject(const GameObject& obj, GLuint shaderProgram) {
    if (!obj.visible || obj.vao == 0) return;
    
    glm::mat4 model = obj.getModelMatrix();
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), obj.color.r, obj.color.g, obj.color.b);
    
    glBindVertexArray(obj.vao);
    glDrawElements(GL_TRIANGLES, obj.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void renderGrid(GLuint shaderProgram) {
    // Generate grid vertices
    std::vector<float> vertices;
    std::vector<float> colors;
    
    int lines = static_cast<int>(gridSize * 2 + 1);
    float halfSize = gridSize / 2.0f;
    
    // X-axis lines (vertical in XZ plane)
    for (int i = 0; i < lines; i++) {
        float x = -halfSize + i;
        
        // Choose color based on position
        glm::vec3 color = (x == 0) ? glm::vec3(1.0f, 0.3f, 0.3f) : 
                         (fmod(x, 5.0f) == 0) ? glm::vec3(0.5f, 0.5f, 0.5f) : 
                                                glm::vec3(0.3f, 0.3f, 0.3f);
        
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(-halfSize);
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(halfSize);
        
        colors.push_back(color.x); colors.push_back(color.y); colors.push_back(color.z);
        colors.push_back(color.x); colors.push_back(color.y); colors.push_back(color.z);
    }
    
    // Z-axis lines (horizontal in XZ plane)
    for (int i = 0; i < lines; i++) {
        float z = -halfSize + i;
        
        // Choose color based on position
        glm::vec3 color = (z == 0) ? glm::vec3(0.3f, 0.3f, 1.0f) : 
                         (fmod(z, 5.0f) == 0) ? glm::vec3(0.5f, 0.5f, 0.5f) : 
                                                glm::vec3(0.3f, 0.3f, 0.3f);
        
        vertices.push_back(-halfSize); vertices.push_back(0.0f); vertices.push_back(z);
        vertices.push_back(halfSize); vertices.push_back(0.0f); vertices.push_back(z);
        
        colors.push_back(color.x); colors.push_back(color.y); colors.push_back(color.z);
        colors.push_back(color.x); colors.push_back(color.y); colors.push_back(color.z);
    }
    
    // Create VAO and VBOs
    static GLuint gridVAO = 0, gridVBO = 0, gridColorVBO = 0;
    if (gridVAO == 0) {
        glGenVertexArrays(1, &gridVAO);
        glGenBuffers(1, &gridVBO);
        glGenBuffers(1, &gridColorVBO);
        
        glBindVertexArray(gridVAO);
        
        // Position buffer
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        
        // Color buffer
        glBindBuffer(GL_ARRAY_BUFFER, gridColorVBO);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
    }
    
    // Update data if grid size changed
    static float lastGridSize = 0;
    if (lastGridSize != gridSize) {
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ARRAY_BUFFER, gridColorVBO);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);
        
        lastGridSize = gridSize;
    }
    
    // Render grid
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size() / 3));
    glBindVertexArray(0);
}

void renderAxes(GLuint shaderProgram) {
    // Axis lines: X=red, Y=green, Z=blue
    float vertices[] = {
        0.0f, 0.0f, 0.0f,  // Origin
        3.0f, 0.0f, 0.0f,  // X-axis
        0.0f, 0.0f, 0.0f,  // Origin
        0.0f, 3.0f, 0.0f,  // Y-axis
        0.0f, 0.0f, 0.0f,  // Origin
        0.0f, 0.0f, 3.0f   // Z-axis
    };
    
    float colors[] = {
        1.0f, 0.3f, 0.3f,  // X-axis start
        1.0f, 0.3f, 0.3f,  // X-axis end
        0.3f, 1.0f, 0.3f,  // Y-axis start
        0.3f, 1.0f, 0.3f,  // Y-axis end
        0.3f, 0.3f, 1.0f,  // Z-axis start
        0.3f, 0.3f, 1.0f   // Z-axis end
    };
    
    // Create VAO and VBOs
    static GLuint axesVAO = 0, axesVBO = 0, axesColorVBO = 0;
    if (axesVAO == 0) {
        glGenVertexArrays(1, &axesVAO);
        glGenBuffers(1, &axesVBO);
        glGenBuffers(1, &axesColorVBO);
        
        glBindVertexArray(axesVAO);
        
        // Position buffer
        glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        
        // Color buffer
        glBindBuffer(GL_ARRAY_BUFFER, axesColorVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
    }
    
    // Render axes with thicker lines
    glLineWidth(2.0f);
    glBindVertexArray(axesVAO);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void renderGizmo(GLuint shaderProgram) {
    if (selectedObjectIndex < 0 || selectedObjectIndex >= objects.size()) return;
    
    auto& obj = objects[selectedObjectIndex];
    glm::mat4 model = obj->getModelMatrix();
    
    // Create gizmo geometry based on transform mode
    std::vector<float> vertices;
    std::vector<float> colors;
    
    float size = gizmoSize;
    
    if (transformMode == TRANSFORM_TRANSLATE) {
        // Translation arrows
        // X axis (red)
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(size); vertices.push_back(0.0f); vertices.push_back(0.0f);
        colors.push_back(1.0f); colors.push_back(0.0f); colors.push_back(0.0f);
        colors.push_back(1.0f); colors.push_back(0.0f); colors.push_back(0.0f);
        
        // Y axis (green)
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(0.0f); vertices.push_back(size); vertices.push_back(0.0f);
        colors.push_back(0.0f); colors.push_back(1.0f); colors.push_back(0.0f);
        colors.push_back(0.0f); colors.push_back(1.0f); colors.push_back(0.0f);
        
        // Z axis (blue)
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(size);
        colors.push_back(0.0f); colors.push_back(0.0f); colors.push_back(1.0f);
        colors.push_back(0.0f); colors.push_back(0.0f); colors.push_back(1.0f);
    } else if (transformMode == TRANSFORM_ROTATE) {
        // Rotation circles
        int segments = 32;
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / (float)segments * 2.0f * glm::pi<float>();
            
            // XY plane (blue)
            float x = cos(angle) * size;
            float y = sin(angle) * size;
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(0.0f);
            colors.push_back(0.0f); colors.push_back(0.0f); colors.push_back(1.0f);
            
            // XZ plane (green)
            x = cos(angle) * size;
            float z = sin(angle) * size;
            vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(z);
            colors.push_back(0.0f); colors.push_back(1.0f); colors.push_back(0.0f);
            
            // YZ plane (red)
            y = cos(angle) * size;
            z = sin(angle) * size;
            vertices.push_back(0.0f); vertices.push_back(y); vertices.push_back(z);
            colors.push_back(1.0f); colors.push_back(0.0f); colors.push_back(0.0f);
        }
    } else if (transformMode == TRANSFORM_SCALE) {
        // Scale cubes
        float cubeSize = size * 0.1f;
        
        // X axis (red cube)
        vertices.push_back(size - cubeSize); vertices.push_back(-cubeSize); vertices.push_back(-cubeSize);
        vertices.push_back(size + cubeSize); vertices.push_back(cubeSize); vertices.push_back(cubeSize);
        colors.push_back(1.0f); colors.push_back(0.0f); colors.push_back(0.0f);
        colors.push_back(1.0f); colors.push_back(0.0f); colors.push_back(0.0f);
        
        // Y axis (green cube)
        vertices.push_back(-cubeSize); vertices.push_back(size - cubeSize); vertices.push_back(-cubeSize);
        vertices.push_back(cubeSize); vertices.push_back(size + cubeSize); vertices.push_back(cubeSize);
        colors.push_back(0.0f); colors.push_back(1.0f); colors.push_back(0.0f);
        colors.push_back(0.0f); colors.push_back(1.0f); colors.push_back(0.0f);
        
        // Z axis (blue cube)
        vertices.push_back(-cubeSize); vertices.push_back(-cubeSize); vertices.push_back(size - cubeSize);
        vertices.push_back(cubeSize); vertices.push_back(cubeSize); vertices.push_back(size + cubeSize);
        colors.push_back(0.0f); colors.push_back(0.0f); colors.push_back(1.0f);
        colors.push_back(0.0f); colors.push_back(0.0f); colors.push_back(1.0f);
    }
    
    // Create VAO and VBOs
    static GLuint gizmoVAO = 0, gizmoVBO = 0, gizmoColorVBO = 0;
    if (gizmoVAO == 0) {
        glGenVertexArrays(1, &gizmoVAO);
        glGenBuffers(1, &gizmoVBO);
        glGenBuffers(1, &gizmoColorVBO);
    }
    
    glBindVertexArray(gizmoVAO);
    
    // Position buffer
    glBindBuffer(GL_ARRAY_BUFFER, gizmoVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color buffer
    glBindBuffer(GL_ARRAY_BUFFER, gizmoColorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);
    
    // Set model matrix
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    
    // Render gizmo
    glLineWidth(2.0f);
    if (transformMode == TRANSFORM_ROTATE) {
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(vertices.size() / 3));
    } else {
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size() / 3));
    }
    glLineWidth(1.0f);
    
    glBindVertexArray(0);
}

void updateCamera() {
    // Limit camera pitch
    const float maxPitch = glm::pi<float>() / 2.0f - 0.01f;
    const float minPitch = -maxPitch;
    
    if (cameraPitch > maxPitch) cameraPitch = maxPitch;
    if (cameraPitch < minPitch) cameraPitch = minPitch;
    
    // Keep camera distance in bounds
    if (cameraDistance < 0.5f) cameraDistance = 0.5f;
    if (cameraDistance > 100.0f) cameraDistance = 100.0f;
}

void showMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                // Clear all objects
                for (auto& obj : objects) {
                    if (obj->vao) glDeleteVertexArrays(1, &obj->vao);
                    if (obj->vbo) glDeleteBuffers(1, &obj->vbo);
                    if (obj->ebo) glDeleteBuffers(1, &obj->ebo);
                }
                objects.clear();
                selectedObjectIndex = -1;
                cameraTarget = glm::vec3(0.0f);
                cameraDistance = 10.0f;
                snprintf(statusMessage, sizeof(statusMessage), "New scene created");
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Import OBJ...", "Ctrl+O")) {
                std::string path = openFileDialog("OBJ Files\0*.obj\0All Files\0*.*\0");
                if (!path.empty()) {
                    loadOBJModel(path.c_str());
                }
            }
            
            if (ImGui::MenuItem("Import GLB...", "Ctrl+Shift+O")) {
                std::string path = openFileDialog("GLB Files\0*.glb\0All Files\0*.*\0");
                if (!path.empty()) {
                    loadGLBModel(path.c_str());
                }
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, selectedObjectIndex >= 0)) {
                if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
                    auto original = objects[selectedObjectIndex];
                    auto copy = std::make_shared<GameObject>(*original);
                    
                    // Generate new OpenGL buffers for the copy
                    glGenVertexArrays(1, &copy->vao);
                    glGenBuffers(1, &copy->vbo);
                    glGenBuffers(1, &copy->ebo);
                    
                    // Copy the buffer data (in a real implementation, you'd copy the actual data)
                    copy->position.x += 1.0f; // Offset the copy
                    
                    char newName[256];
                    snprintf(newName, sizeof(newName), "%s Copy", original->name);
                    strcpy_s(copy->name, sizeof(copy->name), newName);
                    
                    objects.push_back(copy);
                    selectedObjectIndex = objects.size() - 1;
                    
                    snprintf(statusMessage, sizeof(statusMessage), "Duplicated object: %s", original->name);
                }
            }
            
            if (ImGui::MenuItem("Delete", "Del", false, selectedObjectIndex >= 0)) {
                if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
                    auto obj = objects[selectedObjectIndex];
                    snprintf(statusMessage, sizeof(statusMessage), "Deleted object: %s", obj->name);
                    
                    // Delete OpenGL resources
                    if (obj->vao) glDeleteVertexArrays(1, &obj->vao);
                    if (obj->vbo) glDeleteBuffers(1, &obj->vbo);
                    if (obj->ebo) glDeleteBuffers(1, &obj->ebo);
                    
                    objects.erase(objects.begin() + selectedObjectIndex);
                    selectedObjectIndex = -1;
                }
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Center All Models")) {
                centerAllModels();
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Wireframe", "W", &wireframeMode);
            ImGui::MenuItem("Show Grid", "G", &showGrid);
            ImGui::MenuItem("Show Axes", "A", &showAxes);
            ImGui::MenuItem("Show Bounding Boxes", "B", &showBoundingBoxes);
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Reset Camera", "R")) {
                cameraDistance = 10.0f;
                cameraYaw = 0.0f;
                cameraPitch = 0.3f;
                cameraTarget = glm::vec3(0.0f);
                snprintf(statusMessage, sizeof(statusMessage), "Camera reset");
            }
            
            if (ImGui::MenuItem("Frame Selection", "F", false, selectedObjectIndex >= 0)) {
                autoCenterSelectedModel();
            }
            
            ImGui::Separator();
            
            ImGui::MenuItem("Show Object List", NULL, &showObjectListWindow);
            ImGui::MenuItem("Show Transform", NULL, &showTransformWindow);
            ImGui::MenuItem("Show Lighting", NULL, &showLightWindow);
            ImGui::MenuItem("Show Stats", NULL, &showStatsWindow);
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Cube")) {
                createPrimitive("Cube");
                snprintf(statusMessage, sizeof(statusMessage), "Created cube");
            }
            
            if (ImGui::MenuItem("Sphere")) {
                createPrimitive("Sphere");
                snprintf(statusMessage, sizeof(statusMessage), "Created sphere");
            }
            
            if (ImGui::MenuItem("Cylinder")) {
                createPrimitive("Cylinder");
                snprintf(statusMessage, sizeof(statusMessage), "Created cylinder");
            }
            
            if (ImGui::MenuItem("Cone")) {
                createPrimitive("Cone");
                snprintf(statusMessage, sizeof(statusMessage), "Created cone");
            }
            
            if (ImGui::MenuItem("Plane")) {
                createPrimitive("Plane");
                snprintf(statusMessage, sizeof(statusMessage), "Created plane");
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                ImGui::OpenPopup("About");
            }
            
            if (ImGui::MenuItem("Controls")) {
                ImGui::OpenPopup("Controls");
            }
            
            ImGui::EndMenu();
        }
        
        // About popup
        if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("3D Model Viewer Pro");
            ImGui::Text("Version 1.0.0");
            ImGui::Separator();
            ImGui::Text("A professional 3D model viewer and editor");
            ImGui::Text("Built with OpenGL, GLFW, and Dear ImGui");
            ImGui::Separator();
            
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        // Controls popup
        if (ImGui::BeginPopupModal("Controls", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Camera Controls:");
            ImGui::BulletText("Left Click + Drag: Rotate camera");
            ImGui::BulletText("Right Click + Drag: Pan camera");
            ImGui::BulletText("Mouse Wheel: Zoom in/out");
            ImGui::BulletText("W/A/S/D: Move camera");
            ImGui::BulletText("Q/E: Move camera up/down");
            ImGui::Separator();
            
            ImGui::Text("Object Controls:");
            ImGui::BulletText("Click object: Select object");
            ImGui::BulletText("G: Move mode");
            ImGui::BulletText("R: Rotate mode");
            ImGui::BulletText("S: Scale mode");
            ImGui::BulletText("Delete: Delete selected object");
            ImGui::Separator();
            
            ImGui::Text("View Controls:");
            ImGui::BulletText("W: Toggle wireframe");
            ImGui::BulletText("G: Toggle grid");
            ImGui::BulletText("A: Toggle axes");
            ImGui::BulletText("F: Frame selected object");
            ImGui::BulletText("R: Reset camera");
            
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void showLeftPanel() {
    if (!showObjectListWindow && !showTransformWindow) return;
    
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    float menuBarHeight = ImGui::GetFrameHeight();
    float statusBarHeight = 20.0f;
    float leftPanelWidth = 280.0f;
    float margin = 10.0f;
    
    // Left panel - Object List and Transform
    ImGui::SetNextWindowPos(ImVec2(margin, menuBarHeight + margin));
    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, 
        mainViewport->Size.y - menuBarHeight - statusBarHeight - 2 * margin));
    
    if (ImGui::Begin("Left Panel", NULL, 
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
        
        if (showObjectListWindow) {
            if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (objects.empty()) {
                    ImGui::TextColored(COLOR_TEXT_DIM, "No objects in scene");
                    ImGui::TextColored(COLOR_TEXT_DIM, "Use Create menu to add objects");
                } else {
                    for (size_t i = 0; i < objects.size(); i++) {
                        ImGui::PushID(static_cast<int>(i));
                        
                        bool isSelected = (selectedObjectIndex == static_cast<int>(i));
                        auto& obj = objects[i];
                        
                        // Create selectable with eye icon for visibility
                        char label[512];
                        const char* icon = obj->visible ? "👁" : "👁‍🗨";
                        snprintf(label, sizeof(label), "%s %s", icon, obj->name);
                        
                        if (ImGui::Selectable(label, isSelected)) {
                            selectedObjectIndex = static_cast<int>(i);
                        }
                        
                        // Right-click context menu
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::MenuItem("Rename")) {
                                ImGui::OpenPopup("Rename Object");
                            }
                            
                            if (ImGui::MenuItem(obj->visible ? "Hide" : "Show")) {
                                obj->visible = !obj->visible;
                            }
                            
                            if (ImGui::MenuItem("Center View")) {
                                autoCenterSelectedModel();
                            }
                            
                            if (ImGui::MenuItem("Delete")) {
                                if (obj->vao) glDeleteVertexArrays(1, &obj->vao);
                                if (obj->vbo) glDeleteBuffers(1, &obj->vbo);
                                if (obj->ebo) glDeleteBuffers(1, &obj->ebo);
                                objects.erase(objects.begin() + i);
                                selectedObjectIndex = -1;
                                ImGui::CloseCurrentPopup();
                            }
                            
                            ImGui::EndPopup();
                        }
                        
                        // Rename popup
                        if (ImGui::BeginPopupModal("Rename Object", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                            static char newName[256];
                            strcpy_s(newName, sizeof(newName), obj->name);
                            
                            ImGui::InputText("Name", newName, sizeof(newName));
                            
                            if (ImGui::Button("OK", ImVec2(120, 0))) {
                                strcpy_s(obj->name, sizeof(obj->name), newName);
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }
                        
                        ImGui::PopID();
                    }
                }
                
                ImGui::Separator();
                if (ImGui::Button("Center All Models", ImVec2(-1, 0))) {
                    centerAllModels();
                }
            }
        }
        
        if (showTransformWindow && selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& obj = objects[selectedObjectIndex];
                
                // Transform mode buttons
                ImGui::Text("Mode:");
                ImGui::SameLine();
                if (ImGui::RadioButton("Move", transformMode == TRANSFORM_TRANSLATE)) {
                    transformMode = TRANSFORM_TRANSLATE;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Rotate", transformMode == TRANSFORM_ROTATE)) {
                    transformMode = TRANSFORM_ROTATE;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Scale", transformMode == TRANSFORM_SCALE)) {
                    transformMode = TRANSFORM_SCALE;
                }
                
                ImGui::Separator();
                
                // Position controls
                ImGui::Text("Position:");
                float pos[3] = { obj->position.x, obj->position.y, obj->position.z };
                if (ImGui::DragFloat3("##Position", pos, 0.1f)) {
                    if (snapToGrid) {
                        obj->position.x = roundf(pos[0] / gridSnapSize) * gridSnapSize;
                        obj->position.y = roundf(pos[1] / gridSnapSize) * gridSnapSize;
                        obj->position.z = roundf(pos[2] / gridSnapSize) * gridSnapSize;
                    } else {
                        obj->position.x = pos[0];
                        obj->position.y = pos[1];
                        obj->position.z = pos[2];
                    }
                }
                
                // Rotation controls (in degrees for user interface)
                ImGui::Text("Rotation:");
                float rot[3] = { 
                    glm::degrees(obj->rotation.x),
                    glm::degrees(obj->rotation.y),
                    glm::degrees(obj->rotation.z)
                };
                if (ImGui::DragFloat3("##Rotation", rot, 1.0f, -180.0f, 180.0f)) {
                    obj->rotation.x = glm::radians(rot[0]);
                    obj->rotation.y = glm::radians(rot[1]);
                    obj->rotation.z = glm::radians(rot[2]);
                }
                
                // Scale controls
                ImGui::Text("Scale:");
                float scale[3] = { obj->scale.x, obj->scale.y, obj->scale.z };
                if (ImGui::DragFloat3("##Scale", scale, 0.01f, 0.01f, 10.0f)) {
                    obj->scale.x = scale[0];
                    obj->scale.y = scale[1];
                    obj->scale.z = scale[2];
                }
                
                ImGui::Separator();
                
                // Color picker
                ImGui::Text("Color:");
                float color[3] = { obj->color.r, obj->color.g, obj->color.b };
                if (ImGui::ColorEdit3("##Color", color, ImGuiColorEditFlags_NoInputs)) {
                    obj->color.r = color[0];
                    obj->color.g = color[1];
                    obj->color.b = color[2];
                }
                
                ImGui::Separator();
                
                // Reset buttons
                if (ImGui::Button("Reset Position", ImVec2(-1, 0))) {
                    obj->position = glm::vec3(0.0f);
                }
                if (ImGui::Button("Reset Rotation", ImVec2(-1, 0))) {
                    obj->rotation = glm::vec3(0.0f);
                }
                if (ImGui::Button("Reset Scale", ImVec2(-1, 0))) {
                    obj->scale = glm::vec3(1.0f);
                }
                if (ImGui::Button("Center View", ImVec2(-1, 0))) {
                    autoCenterSelectedModel();
                }
            }
        }
    }
    ImGui::End();
}

void showRightPanel() {
    if (!showLightWindow && !showStatsWindow) return;
    
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    float menuBarHeight = ImGui::GetFrameHeight();
    float statusBarHeight = 20.0f;
    float rightPanelWidth = 280.0f;
    float margin = 10.0f;
    
    // Right panel - Lighting and Stats
    ImGui::SetNextWindowPos(ImVec2(
        mainViewport->Size.x - rightPanelWidth - margin,
        menuBarHeight + margin
    ));
    ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, 
        mainViewport->Size.y - menuBarHeight - statusBarHeight - 2 * margin));
    
    if (ImGui::Begin("Right Panel", NULL, 
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
        
        if (showLightWindow) {
            if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Light Position:");
                ImGui::DragFloat3("##LightPos", &lightPos.x, 0.1f, -20.0f, 20.0f);
                
                ImGui::Separator();
                
                ImGui::Text("Light Color:");
                ImGui::ColorEdit3("##LightColor", lightColor, ImGuiColorEditFlags_NoInputs);
                
                ImGui::Separator();
                
                ImGui::Text("Background Color:");
                ImGui::ColorEdit3("##BgColor", backgroundColor, ImGuiColorEditFlags_NoInputs);
                
                ImGui::Separator();
                
                ImGui::Text("Grid Settings:");
                ImGui::DragFloat("Grid Size", &gridSize, 1.0f, 5.0f, 100.0f);
                ImGui::DragFloat("Gizmo Size", &gizmoSize, 0.1f, 0.1f, 5.0f);
                ImGui::Checkbox("Snap to Grid", &snapToGrid);
                if (snapToGrid) {
                    ImGui::DragFloat("Snap Size", &gridSnapSize, 0.1f, 0.1f, 5.0f);
                }
            }
        }
        
        if (showStatsWindow) {
            if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Scene Statistics:");
                ImGui::Separator();
                
                ImGui::Text("Objects: %zu", objects.size());
                
                int totalVertices = 0;
                int totalTriangles = 0;
                for (const auto& obj : objects) {
                    if (obj->visible) {
                        totalVertices += obj->vertexCount;
                        totalTriangles += obj->indexCount / 3;
                    }
                }
                
                ImGui::Text("Visible Vertices: %d", totalVertices);
                ImGui::Text("Visible Triangles: %d", totalTriangles);
                
                if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
                    auto& obj = objects[selectedObjectIndex];
                    ImGui::Separator();
                    ImGui::Text("Selected Object:");
                    ImGui::Text("Vertices: %d", obj->vertexCount);
                    ImGui::Text("Triangles: %d", obj->indexCount / 3);
                    
                    glm::vec3 bboxSize = obj->bboxMax - obj->bboxMin;
                    ImGui::Text("Bounding Box:");
                    ImGui::Text("Size: %.2f x %.2f x %.2f", 
                        bboxSize.x, bboxSize.y, bboxSize.z);
                }
                
                ImGui::Separator();
                ImGui::Text("Performance:");
                ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
                ImGui::Text("Frame Time: %.2f ms", 1000.0f / ImGui::GetIO().Framerate);
            }
        }
    }
    ImGui::End();
}

void showViewport() {
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    float menuBarHeight = ImGui::GetFrameHeight();
    float margin = 10.0f;
    float leftPanelWidth = 280.0f;
    float rightPanelWidth = 280.0f;
    
    // Calculate viewport position and size
    ImVec2 vpPos = ImVec2(margin + leftPanelWidth, menuBarHeight + margin);
    ImVec2 vpSize = ImVec2(
        mainViewport->Size.x - (leftPanelWidth + rightPanelWidth + 3 * margin),
        mainViewport->Size.y - (menuBarHeight + 20.0f + 2 * margin) // 20 for status bar
    );
    
    // Viewport window
    ImGui::SetNextWindowPos(vpPos);
    ImGui::SetNextWindowSize(vpSize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    
    if (ImGui::Begin("Viewport", NULL, 
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        
        isViewportHovered = ImGui::IsWindowHovered();
        isViewportFocused = ImGui::IsWindowFocused();
        
        // Get the actual content region available
        ImVec2 contentSize = ImGui::GetContentRegionAvail();
        
        // Display the rendered texture
        if (viewportTexture != 0) {
            ImGui::Image((void*)(intptr_t)viewportTexture, contentSize, ImVec2(0, 1), ImVec2(1, 0));
        } else {
            ImGui::TextColored(COLOR_TEXT_DIM, "Viewport not initialized");
        }
        
        // Display viewport info overlay
        ImGui::SetCursorPos(ImVec2(10, 10));
        ImGui::BeginChild("Viewport Overlay", ImVec2(200, 120), true, 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
        
        ImGui::Text("Viewport");
        ImGui::Separator();
        
        if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
            auto& obj = objects[selectedObjectIndex];
            ImGui::Text("Selected: %s", obj->name);
        } else {
            ImGui::Text("No selection");
        }
        
        const char* modeText = "";
        switch (transformMode) {
            case TRANSFORM_TRANSLATE: modeText = "Move"; break;
            case TRANSFORM_ROTATE: modeText = "Rotate"; break;
            case TRANSFORM_SCALE: modeText = "Scale"; break;
        }
        ImGui::Text("Mode: %s", modeText);
        
        ImGui::Text("Camera: %.1f units", cameraDistance);
        
        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void showStatusBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - 20));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 2));
    
    if (ImGui::Begin("Status Bar", NULL, 
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | 
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus)) {
        
        ImGui::Text("%s", statusMessage);
        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    }
    
    ImGui::End();
    ImGui::PopStyleVar(2);
}

std::string formatFileSize(size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double s = static_cast<double>(size);
    
    while (s >= 1024 && unit < 4) {
        s /= 1024;
        unit++;
    }
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.1f %s", s, units[unit]);
    return std::string(buffer);
}

std::string formatTime(time_t time) {
    char buffer[32];
    struct tm tm_info;
    
    #ifdef _WIN32
        localtime_s(&tm_info, &time);
    #else
        localtime_r(&time, &tm_info);
    #endif
    
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);
    return std::string(buffer);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (!isViewportHovered) return;
    
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            isMouseDragging = true;
            dragButton = button;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            lastMousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
        } else if (action == GLFW_RELEASE) {
            isMouseDragging = false;
            dragButton = -1;
        }
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!isMouseDragging || !isViewportHovered) return;
    
    glm::vec2 currentMousePos(static_cast<float>(xpos), static_cast<float>(ypos));
    glm::vec2 delta = currentMousePos - lastMousePos;
    
    if (dragButton == GLFW_MOUSE_BUTTON_LEFT) {
        // Rotate camera
        cameraYaw -= delta.x * 0.01f;
        cameraPitch -= delta.y * 0.01f;
        cameraPitch = glm::clamp(cameraPitch, -glm::pi<float>()/2 + 0.1f, glm::pi<float>()/2 - 0.1f);
    } else if (dragButton == GLFW_MOUSE_BUTTON_RIGHT) {
        // Pan camera
        float panSpeed = cameraDistance * 0.002f;
        cameraTarget.x -= delta.x * panSpeed;
        cameraTarget.y += delta.y * panSpeed;
    }
    
    lastMousePos = currentMousePos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (!isViewportHovered) return;
    
    // Zoom camera
    float zoomSpeed = cameraDistance * 0.1f;
    cameraDistance -= static_cast<float>(yoffset) * zoomSpeed;
    cameraDistance = glm::clamp(cameraDistance, 0.5f, 100.0f);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    
    if (!isViewportHovered && !isViewportFocused) return;
    
    // Camera movement with WASD
    float moveSpeed = cameraDistance * 0.1f;
    glm::vec3 cameraForward = glm::normalize(cameraTarget - 
        (cameraTarget + glm::vec3(
            sin(cameraYaw) * cos(cameraPitch) * cameraDistance,
            sin(cameraPitch) * cameraDistance,
            cos(cameraYaw) * cos(cameraPitch) * cameraDistance
        )));
    
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraForward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, cameraForward));
    
    switch (key) {
        case GLFW_KEY_W:
            cameraTarget += cameraForward * moveSpeed;
            break;
        case GLFW_KEY_S:
            cameraTarget -= cameraForward * moveSpeed;
            break;
        case GLFW_KEY_A:
            cameraTarget -= cameraRight * moveSpeed;
            break;
        case GLFW_KEY_D:
            cameraTarget += cameraRight * moveSpeed;
            break;
        case GLFW_KEY_Q:
            cameraTarget += cameraUp * moveSpeed;
            break;
        case GLFW_KEY_E:
            cameraTarget -= cameraUp * moveSpeed;
            break;
            
        // Transform mode shortcuts
        case GLFW_KEY_G:
            transformMode = TRANSFORM_TRANSLATE;
            snprintf(statusMessage, sizeof(statusMessage), "Move mode");
            break;
        case GLFW_KEY_R:
            transformMode = TRANSFORM_ROTATE;
            snprintf(statusMessage, sizeof(statusMessage), "Rotate mode");
            break;
        case GLFW_KEY_T:
            transformMode = TRANSFORM_SCALE;
            snprintf(statusMessage, sizeof(statusMessage), "Scale mode");
            break;
            
        // View shortcuts
        case GLFW_KEY_F:
            autoCenterSelectedModel();
            break;
            
        case GLFW_KEY_DELETE:
            if (selectedObjectIndex >= 0 && selectedObjectIndex < objects.size()) {
                auto obj = objects[selectedObjectIndex];
                snprintf(statusMessage, sizeof(statusMessage), "Deleted object: %s", obj->name);
                
                if (obj->vao) glDeleteVertexArrays(1, &obj->vao);
                if (obj->vbo) glDeleteBuffers(1, &obj->vbo);
                if (obj->ebo) glDeleteBuffers(1, &obj->ebo);
                
                objects.erase(objects.begin() + selectedObjectIndex);
                selectedObjectIndex = -1;
            }
            break;
            
        case GLFW_KEY_C:
            if (mods & GLFW_MOD_CONTROL) {
                centerAllModels();
            }
            break;
    }
}

void drop_callback(GLFWwindow* window, int count, const char** paths) {
    for (int i = 0; i < count; i++) {
        const char* path = paths[i];
        const char* ext = strrchr(path, '.');
        
        if (ext) {
            ext++; // Skip the dot
            
            if (strcasecmp(ext, "obj") == 0) {
                loadOBJModel(path);
            } else if (strcasecmp(ext, "glb") == 0 || strcasecmp(ext, "gltf") == 0) {
                loadGLBModel(path);
            } else {
                snprintf(statusMessage, sizeof(statusMessage), 
                         "Unsupported file format: %s", ext);
            }
        }
    }
}

std::string openFileDialog(const char* filter) {
    #ifdef _WIN32
        OPENFILENAMEA ofn;
        char fileName[MAX_PATH] = "";
        
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = glfwGetWin32Window(window);
        ofn.lpstrFilter = filter;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        ofn.lpstrDefExt = "";
        
        if (GetOpenFileNameA(&ofn)) {
            return std::string(fileName);
        }
    #endif
    
    return "";
}
