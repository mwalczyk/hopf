#include <iostream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "hopf.h"
#include "shader.h"
#include "utils.h"

// References:
// [1](https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions#glframebuffer)

float theta = 0.0f;
float psi = 0.0f;
glm::vec3 camera_position = glm::vec3(0.0f, 1.0f, 3.0f);
glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

void process_input(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    const float speed = 0.025f; // adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        psi += speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        psi -= speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        theta += speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        theta -= speed;

    theta = std::min(glm::pi<float>(), theta);
    theta = std::max(0.0f, theta);

    psi = std::min(glm::two_pi<float>(), psi);
    psi = std::max(0.0f, psi);
    
    camera_position = glm::vec3{
        sinf(theta) * cosf(psi),
        sinf(theta) * sinf(psi),
        cosf(theta)
    } * 3.0f;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param)
{
    auto const src_str = [source]() {
        switch (source)
        {
        case GL_DEBUG_SOURCE_API: return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
        case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
        case GL_DEBUG_SOURCE_OTHER: return "OTHER";
        }
    }();

    auto const type_str = [type]() {
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR: return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER: return "MARKER";
        case GL_DEBUG_TYPE_OTHER: return "OTHER";
        }
    }();

    auto const severity_str = [severity]() {
        switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
        case GL_DEBUG_SEVERITY_LOW: return "LOW";
        case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
        case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
        }
    }();

    std::cout << src_str << ", " << type_str << ", " << severity_str << ", " << id << ": " << message << '\n';
}

std::vector<Vertex> calculate_base_points(float radius = 0.5f, const glm::mat4& transform = glm::mat4{ 1.0f })
{
    std::vector<Vertex> base_points;
    static const auto thetas = linear_spacing(0.0f, glm::two_pi<float>(), 80);

    for (const auto theta : thetas)
    {
        float t = 0.0f;

        float c = cosf(theta + t) * radius;
        float s = sinf(theta + t) * radius;

        glm::vec3 position = glm::vec3{ transform * glm::vec4{ c, s, radius * 2.0f - 1.0f, 1.0f } };

        Vertex vertex;

        vertex.position = position;
        vertex.color = vertex.position * 0.5f + 0.5f;
        vertex.texture_coordinate = { 0.0f, 0.0f };

        base_points.push_back(vertex);
    }

    return base_points;
}

const uint32_t width = 1080;
const uint32_t height = 1080;
const uint32_t ui_w = 256;
const uint32_t ui_h = 256;

int main()
{
    // Create GLFW window 
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(width, height, "ImGui Example", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load function pointers from glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // Initialize ImGui
    const char* glsl_version = "#version 460";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup initial OpenGL state
    {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(message_callback, nullptr);

        glEnable(GL_PRIMITIVE_RESTART);
        glPrimitiveRestartIndex(65535);

        glEnable(GL_PROGRAM_POINT_SIZE);

        glLineWidth(2.0f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    // Load shaders, meshes, etc.
    auto shader_hopf = Shader{ "../shaders/hopf.vert", "../shaders/hopf.frag" };
    auto shader_ui = Shader{ "../shaders/ui.vert", "../shaders/ui.frag" };
    
    std::vector<Vertex> base_points = calculate_base_points();
    auto mesh_base_points = Mesh{ base_points, { /* No indices */ } };
    auto hopf = Hopf{ base_points };

    auto mesh_sphere = Mesh::from_sphere(0.45f, glm::vec3{ 0.0f, 0.0f, 0.0f }, 20, 20);
    auto mesh_grid = Mesh::from_grid(2.0f, 2.0f, glm::vec3{ 0.0f, -0.5f, 0.0f });

    // Create the framebuffer that we will render the S2 sphere into
    uint32_t framebuffer_ui;
    uint32_t texture_color_attachment_ui;
    uint32_t renderbuffer_ui;
    {
        glCreateFramebuffers(1, &framebuffer_ui);

        // Create a color attachment texture and associate it with the framebuffer
        glCreateTextures(GL_TEXTURE_2D, 1, &texture_color_attachment_ui);
        glTextureStorage2D(texture_color_attachment_ui, 1, GL_RGBA8, width, height);
        glTextureParameteri(texture_color_attachment_ui, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture_color_attachment_ui, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glNamedFramebufferTexture(framebuffer_ui, GL_COLOR_ATTACHMENT0, texture_color_attachment_ui, 0);

        // Create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        glCreateRenderbuffers(1, &renderbuffer_ui);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_ui);
        glNamedRenderbufferStorage(renderbuffer_ui, GL_DEPTH24_STENCIL8, width, height);
        glNamedFramebufferRenderbuffer(framebuffer_ui, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer_ui);

        // Now that we actually created the framebuffer and added all attachments we want to check if it is actually complete 
        if (glCheckNamedFramebufferStatus(framebuffer_ui, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error: framebuffer is not complete\n";
        }
    }

    // Values affected by the GUI
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    float radius = 0.5f;
    float rotation_x = 0.0f;
    float rotation_y = 0.0f;
    float rotation_z = 0.0f;
    float last_radius = radius;
    float last_rotation_x = rotation_x;
    float last_rotation_y = rotation_y;
    float last_rotation_z = rotation_z;
    bool auto_scene_rotate = true;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        process_input(window);

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Controls");                          
            ImGui::Text("Hopf Fibration"); 
            ImGui::SliderFloat("Radius", &radius, 0.01f, 0.5f);
            ImGui::SliderFloat("Rotation X", &rotation_x, 0.0f, 360.0f);
            ImGui::SliderFloat("Rotation Y", &rotation_y, 0.0f, 360.0f);
            ImGui::SliderFloat("Rotation Z", &rotation_z, 0.0f, 360.0f);
            ImGui::ColorEdit3("Background Clear Color", (float*)&clear_color);
            ImGui::Text("Application Average %.3f MS/Frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Image((void*)(intptr_t)texture_color_attachment_ui, ImVec2(ui_w, ui_h));
            ImGui::End();
        }
        ImGui::Render();

        // Grab the width and height of the default (on-screen) framebuffer, as it may have changed
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // TODO: handle window resizing...
        // ...
        
        // Set up matrices
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(width) / static_cast<float>(height),
            0.1f,
            1000.0f
        );

        glm::mat4 model{ 1.0f };
        model = glm::rotate(model, rotation_x * (glm::pi<float>() / 180.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });
        model = glm::rotate(model, rotation_y * (glm::pi<float>() / 180.0f), glm::vec3{ 0.0f, 1.0f, 0.0f });
        model = glm::rotate(model, rotation_z * (glm::pi<float>() / 180.0f), glm::vec3{ 0.0f, 0.0f, 1.0f });

        if (last_radius != radius ||
            last_rotation_x != rotation_x || 
            last_rotation_y != rotation_y || 
            last_rotation_z != rotation_z)
        {
            std::vector<Vertex> base_points = calculate_base_points(radius, model);
            mesh_base_points.set_vertices(base_points);

            hopf = Hopf{ base_points };

            // Set prev to next
            last_radius = radius;
            last_rotation_x = rotation_x;
            last_rotation_y = rotation_y;
            last_rotation_z = rotation_z;
        }

        // Render 3D objects to UI (offscreen) framebuffer
        {
            glViewport(0, 0, width, height);
            glEnable(GL_DEPTH_TEST);

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_ui);

            const float clear_color_values[] = { 0.1f, 0.1f, 0.1f, 1.0f };
            const float clear_depth_value = 1.0f;
            const uint32_t color_buffer_index = 0;
            glClearNamedFramebufferfv(framebuffer_ui, GL_COLOR, color_buffer_index, clear_color_values);
            glClearNamedFramebufferfv(framebuffer_ui, GL_DEPTH, 0, &clear_depth_value);
            
            glm::mat4 view = glm::lookAt(
                glm::vec3{ 0.0f, 0.0f, 3.0f },
                glm::vec3{ 0.0f, 0.0f, 0.0f },
                glm::vec3{ 0.0f, 1.0f, 0.0f }
            );

            shader_ui.use();
            shader_ui.uniform_float("u_time", glfwGetTime());
            shader_ui.uniform_mat4("u_projection", projection);
            shader_ui.uniform_mat4("u_view", view);

            shader_ui.uniform_mat4("u_model", model);
            shader_ui.uniform_bool("u_alpha", false);
            mesh_base_points.draw(GL_POINTS);

            // Reset model matrix
            glm::mat4 model{ 1.0f };
            shader_ui.uniform_mat4("u_model", model);
            shader_ui.uniform_bool("u_alpha", true);
            mesh_sphere.draw();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Render 3D objects to default framebuffer
        {
            glViewport(0, 0, width, height);
            glDisable(GL_DEPTH_TEST);

            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);

            glm::mat4 view = glm::lookAt(camera_position, glm::vec3{ 0.0f }, camera_up);//camera_position + camera_front

            shader_hopf.use();
            shader_hopf.uniform_float("u_time", glfwGetTime());
            shader_hopf.uniform_mat4("u_projection", projection);
            shader_hopf.uniform_mat4("u_view", view);
            shader_hopf.uniform_mat4("u_model", model);

            hopf.draw();

            mesh_grid.draw();
        }

        // Render UI
        {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(window);
    }

    // Clean-up ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
       
    // Clean-up GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
}

