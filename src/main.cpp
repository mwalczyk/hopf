#include <iostream>
#include <random>

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
// [2](https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball)

// Data that will be associated with the GLFW window
struct InputData
{
    bool imgui_active = false;
};

// Viewport and camera details
const uint32_t window_w = 1080;
const uint32_t window_h = 1080;
const uint32_t ui_w = 256;
const uint32_t ui_h = 256;
bool first_mouse = true;
float last_x;
float last_y;
glm::mat4 arcball_camera_matrix = glm::lookAt(glm::vec3{ 6.0f, 1.0f, 0.0f }, glm::vec3{ 0.0f }, glm::vec3{ 1.0f, 1.0f, 0.0f });
glm::mat4 arcball_model_matrix = glm::mat4{ 1.0f };


// Global settings
size_t number_of_fibers = 100;
const std::vector<std::string> modes = { "Great Circle", "Random", "Loxodrome" };
std::string current_mode = modes[0];

// Per-mode settings
uint32_t number_of_circles = 1;                             // For mode: "Great Circle"
std::vector<float> offsets = { 0.0f };                      // For mode: "Great Circle"
std::vector<float> arc_angles = { glm::two_pi<float>() };   // For mode: "Great Circle"
float rotation_x = 0.0f;                                    // For mode: "Great Circle"
float rotation_y = 0.0f;                                    // For mode: "Great Circle"
float rotation_z = 0.0f;                                    // For mode: "Great Circle"
uint32_t seed = 0.0f;                                       // For mode: "Random"
float mean = 0.0f;                                          // For mode: "Random"
float standard_deviation = 1.0f;                            // For mode: "Random"
float loxodrome_offset = 2.0f;                              // For mode: "Loxodrome"

// Appearance
static char filename[64] = "Hopf.obj";
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);   
bool show_floor_plane = true;
bool draw_as_points = false;

InputData input_data;

/**
 * A function for handling key presses.
 */
void process_input(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
    {
        // Reset the arcball camera
        arcball_camera_matrix = glm::lookAt(glm::vec3{ 6.0f, 1.0f, 0.0f }, glm::vec3{ 0.0f }, glm::vec3{ 1.0f, 1.0f, 0.0f });
        arcball_model_matrix = glm::mat4{ 1.0f };
    }
}

/**
 * Get a normalized vector from the center of the virtual ball `O` to a
 * point `P` on the virtual ball surface, such that `P` is aligned on
 * screen's (X, Y) coordinates.  If (X, Y) is too far away from the
 * sphere, return the nearest point on the virtual ball surface.
 */
glm::vec3 get_arcball_vector(int x, int y) 
{
    glm::vec3 P = glm::vec3(
        1.0 * x / window_w * 2.0f - 1.0f,
        1.0 * y / window_h * 2.0f - 1.0f,
        0.0f
    );

    P.y = -P.y;

    float op_squared = P.x * P.x + P.y * P.y;

    if (op_squared <= 1.0f * 1.0f)
    {
        // Pythagorean theorem
        P.z = sqrt(1.0f * 1.0f - op_squared);  
    }
    else
    {
        // Nearest point
        P = glm::normalize(P);  
    }

    return P;
}

/**
 * Performs arcball camera calculations.
 */
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // First, check if the user is interacting with the ImGui interface - if they are,
    // we don't want to process mouse events any further
    auto input_data = static_cast<InputData*>(glfwGetWindowUserPointer(window));

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !input_data->imgui_active)
    {
        if (first_mouse)
        {
            last_x = xpos;
            last_y = ypos;
            first_mouse = false;
        }

        if (xpos != last_x || ypos != last_y)
        {
            const float rotation_speed = 0.25f;

            glm::vec3 va = get_arcball_vector(last_x, last_y);
            glm::vec3 vb = get_arcball_vector(xpos, ypos);
            const float angle = acos(std::min(1.0f, glm::dot(va, vb))) * rotation_speed;
            const glm::vec3 axis_camera_coordinates = glm::cross(va, vb);

            glm::mat3 camera_to_object = glm::inverse(glm::mat3(arcball_camera_matrix) * glm::mat3(arcball_model_matrix));

            glm::vec3 axis_in_object_coord = camera_to_object * axis_camera_coordinates;

            arcball_model_matrix = glm::rotate(arcball_model_matrix, glm::degrees(angle), axis_in_object_coord);

            // Set last to current
            last_x = xpos;
            last_y = ypos;
        }
    } 
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        last_x = xpos;
        last_y = ypos;
    }
}

/**
 * Debug function that will be used internally by OpenGL to print out warnings, errors, etc.
 */
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

std::vector<Vertex> calculate_base_points_great_circle(const glm::mat4& transform = glm::mat4{ 1.0f })
{
    std::vector<Vertex> base_points;
   
    for (size_t i = 0; i < number_of_circles; ++i)
    {
        const float offset = offsets[i];
        const float arc_angle = arc_angles[i];

        const auto thetas = linear_spacing(0.0f, arc_angle, number_of_fibers);

        for (const auto theta : thetas)
        {
            const float c = cosf(theta) * (1.0f - fabsf(offset));
            const float s = sinf(theta) * (1.0f - fabsf(offset));

            const glm::vec3 position = glm::vec3{ transform * glm::vec4{ c, s, offset, 1.0f } };
        
            Vertex vertex;

            vertex.position = position;
            vertex.color = vertex.position * 0.5f + 0.5f;
            vertex.texture_coordinate = { 0.0f, 0.0f };

            base_points.push_back(vertex);
        }
    }

    return base_points;
}

std::vector<Vertex> calculate_base_points_random(const glm::mat4& transform = glm::mat4{ 1.0f })
{
    std::vector<Vertex> base_points;

    // Create a normal (Gaussian) distribution generator
    std::default_random_engine generator{ seed };
    std::normal_distribution<float> distribution(mean, standard_deviation);

    for (size_t i = 0; i < number_of_fibers; ++i)
    {
        const auto rand_x = distribution(generator);
        const auto rand_y = distribution(generator);
        const auto rand_z = distribution(generator);

        const float radius = 1.0f;

        Vertex vertex;
        vertex.position = glm::vec3{ transform * glm::vec4{ glm::normalize(glm::vec3{ rand_x, rand_y, rand_z }) * radius, 1.0f} };
        vertex.color = vertex.position * 0.5f + 0.5f;
        vertex.texture_coordinate = { 0.0f, 0.0f };

        base_points.push_back(vertex);
    }

    return base_points;
}

std::vector<Vertex> calculate_base_points_loxodrome(const glm::mat4& transform = glm::mat4{ 1.0f })
{
    std::vector<Vertex> base_points;

    // Don't go all the way to `pi / 2` because there are discontinuities at the poles
    auto thetas = linear_spacing(-glm::pi<float>() * 0.45f, glm::pi<float>() * 0.45f, number_of_fibers);

    for (size_t i = 0; i < number_of_fibers; ++i)
    {
       
        const float radius = 1.0f;
        
        const float x = radius * cosf(thetas[i]) * cosf(thetas[i] * loxodrome_offset);
        const float y = radius * cosf(thetas[i]) * sinf(thetas[i] * loxodrome_offset);
        const float z = radius * sinf(thetas[i]);

        Vertex vertex;
        vertex.position = glm::vec3{ transform * glm::vec4{ x, y, z, 1.0f } };
        vertex.color = vertex.position * 0.5f + 0.5f;
        vertex.texture_coordinate = { 0.0f, 0.0f };

        base_points.push_back(vertex);
    }

    return base_points;
}

int main()
{
    // Create and configure the GLFW window 
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, false);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window = glfwCreateWindow(window_w, window_h, "Hopf Fibration", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetWindowUserPointer(window, &input_data);

    // Load function pointers from glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // Setup initial OpenGL state
    {
#if defined(_DEBUG)
        // Debug logging
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(message_callback, nullptr);
#endif

        // Depth testing
        glEnable(GL_DEPTH_TEST);
        
        // Primitive restart (for drawing all fibers via a single VBO)
        glEnable(GL_PRIMITIVE_RESTART);
        glPrimitiveRestartIndex(65535);

        // Program point size (for setting base point draw size in the vertex shader)
        glEnable(GL_PROGRAM_POINT_SIZE);

        // Line width for the fibers
        glLineWidth(2.0f);

        // Alpha blending for the sphere UI
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Backface culling for optimization
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    // Load shaders, meshes, etc.
    auto shader_hopf = Shader{ "../shaders/hopf.vert", "../shaders/hopf.frag" };
    auto shader_ui = Shader{ "../shaders/ui.vert", "../shaders/ui.frag" };
    
    std::vector<Vertex> base_points = calculate_base_points_great_circle();
    auto mesh_base_points = Mesh{ base_points, { /* No indices */ } };
    auto hopf = Hopf{ base_points };

    auto mesh_sphere = Mesh::from_sphere(0.75f, glm::vec3{ 0.0f, 0.0f, 0.0f }, 20, 20);
    auto mesh_grid = Mesh::from_grid(2.0f, 2.0f, glm::vec3{ 0.0f, -0.6f, 0.0f });
    auto mesh_coordinate_frame = Mesh::from_coordinate_frame(10.f);

    // Create the framebuffer that we will render the S2 sphere into
    uint32_t framebuffer_ui;
    uint32_t texture_color_attachment_ui;
    uint32_t renderbuffer_ui;
    {
        glCreateFramebuffers(1, &framebuffer_ui);

        // Create a color attachment texture and associate it with the framebuffer
        glCreateTextures(GL_TEXTURE_2D, 1, &texture_color_attachment_ui);
        glTextureStorage2D(texture_color_attachment_ui, 1, GL_RGBA8, window_w, window_h);
        glTextureParameteri(texture_color_attachment_ui, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture_color_attachment_ui, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glNamedFramebufferTexture(framebuffer_ui, GL_COLOR_ATTACHMENT0, texture_color_attachment_ui, 0);

        // Create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
        glCreateRenderbuffers(1, &renderbuffer_ui);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_ui);
        glNamedRenderbufferStorage(renderbuffer_ui, GL_DEPTH24_STENCIL8, window_w, window_h);
        glNamedFramebufferRenderbuffer(framebuffer_ui, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer_ui);

        // Now that we actually created the framebuffer and added all attachments we want to check if it is actually complete 
        if (glCheckNamedFramebufferStatus(framebuffer_ui, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error: framebuffer is not complete\n";
        }
    }

    while (!glfwWindowShouldClose(window))
    {
        // Update flag that denotes whether or not the user is interacting with ImGui
        input_data.imgui_active = io.WantCaptureMouse;

        // Poll regular GLFW window events
        glfwPollEvents();
        process_input(window);

        // This flag will be set to `true` by the various UI elements if the settings have changed
        // in such a way as to warrant a recalculation of the fibration topology 
        bool topology_needs_update = false;

        // Handle ImGui stuff
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            // Container #1: settings 
            {
                ImGui::Begin("Hopf Fibration");

                // Global settings (shared across modes)
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram), "Primary Controls");
                topology_needs_update |= ImGui::SliderInt("Number of Fibers", (int*)&number_of_fibers, 10, 400);

                if (ImGui::BeginCombo("Mode", current_mode.c_str())) 
                {
                    for (size_t i = 0; i < modes.size(); i++)
                    {
                        bool is_selected = current_mode.c_str() == modes[i];

                        if (ImGui::Selectable(modes[i].c_str(), is_selected))
                        {
                            topology_needs_update |= true;
                            current_mode = modes[i];
                        }
                        if (is_selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::Separator();

                // Per-mode UI settings
                if (current_mode == "Great Circle")
                {
                    ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram), "Per-Fiber Settings");
                    bool number_of_circles_changed = ImGui::SliderInt("Number of Circles", (int*)&number_of_circles, 1, 10);
                    topology_needs_update |= number_of_circles_changed;

                    // Resize radii / arc angle vectors if the user has changed the number of circles
                    if (number_of_circles_changed)
                    {
                        offsets = linear_spacing(0.0f, -0.9f, number_of_circles);
                        arc_angles = linear_spacing((glm::two_pi<float>()) * 0.25f, glm::two_pi<float>() * 0.75f, number_of_circles);
                    }

                    // Draw per-circle sliders with a different color
                    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
                    {
                        for (size_t i = 0; i < number_of_circles; ++i)
                        {
                            const std::string name = "Circle " + std::to_string(i + 1);
                            const std::string offset_name = "Offset##" + std::to_string(i + 1);
                            const std::string arc_angle_name = "Arc Angle##" + std::to_string(i + 1);
                            ImGui::Text(name.c_str());

                            topology_needs_update |= ImGui::SliderFloat(offset_name.c_str(), &offsets[i], -0.99f, 0.99f);
                            topology_needs_update |= ImGui::SliderFloat(arc_angle_name.c_str(), &arc_angles[i], 0.01f, glm::two_pi<float>());
                        }
                    }
                    ImGui::PopStyleColor();
                }
                else if (current_mode == "Random")
                {
                    topology_needs_update |= ImGui::SliderInt("Seed", (int*)&seed, 0, 1000);
                    topology_needs_update |= ImGui::SliderFloat("Mean", &mean, -3.0f, 3.0f);
                    topology_needs_update |= ImGui::SliderFloat("Standard Deviation", &standard_deviation, 0.1f, 3.0f);
                }
                else if (current_mode == "Loxodrome")
                {
                    topology_needs_update |= ImGui::SliderFloat("Loxodrome Offset", &loxodrome_offset, 2.0f, 20.0f);
                }

                // Global rotation applied to all base points in every mode
                ImGui::Separator();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram), "Rotations (Applied to All Fibers)");
                topology_needs_update |= ImGui::SliderFloat("Rotation X", &rotation_x, 0.0f, glm::pi<float>());
                topology_needs_update |= ImGui::SliderFloat("Rotation Y", &rotation_y, 0.0f, glm::pi<float>());
                topology_needs_update |= ImGui::SliderFloat("Rotation Z", &rotation_z, 0.0f, glm::pi<float>());

                ImGui::Separator();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram), "Appearance");
                ImGui::InputText("", filename, 64);
                ImGui::SameLine();
                if (ImGui::Button("Export"))
                {
                    hopf.save_obj(filename);
                }
                ImGui::ColorEdit3("Background Color", (float*)&clear_color);
                ImGui::Checkbox("Show Floor Plane", &show_floor_plane);
                ImGui::Checkbox("Draw as Points (Instead of Lines)", &draw_as_points);
                ImGui::Separator();
                ImGui::Text("Application Average %.3f MS/Frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }
            // Container #2: preview UI
            {
                ImGui::Begin("Mapping (Points on S2)");
                ImGui::Image((void*)(intptr_t)texture_color_attachment_ui, ImVec2(ui_w, ui_h));
                ImGui::End();
            }
        }
        ImGui::Render();

        // The transformation matrix that will be applied to the base points on S2 to generate the fibration
        glm::mat4 model{ 1.0f };
        model = glm::rotate(model, rotation_x, glm::vec3{ 1.0f, 0.0f, 0.0f });
        model = glm::rotate(model, rotation_y, glm::vec3{ 0.0f, 1.0f, 0.0f });
        model = glm::rotate(model, rotation_z, glm::vec3{ 0.0f, 0.0f, 1.0f });

        if (topology_needs_update)
        {
            std::vector<Vertex> base_points;

            if (current_mode == "Great Circle")
            {
                base_points = calculate_base_points_great_circle(model);
            }
            else if (current_mode == "Random")
            {
                base_points = calculate_base_points_random(model);
            }
            else if (current_mode == "Loxodrome")
            {
                base_points = calculate_base_points_loxodrome(model);
            }

            mesh_base_points.set_vertices(base_points);

            hopf = Hopf{ base_points };
        }

        // Render 3D objects to UI (offscreen) framebuffer
        {
            glViewport(0, 0, window_w, window_h);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_ui);

            const float clear_color_values[] = { 0.1f, 0.1f, 0.1f, 1.0f };
            const float clear_depth_value = 1.0f;
            const uint32_t color_buffer_index = 0;
            glClearNamedFramebufferfv(framebuffer_ui, GL_COLOR, color_buffer_index, clear_color_values);
            glClearNamedFramebufferfv(framebuffer_ui, GL_DEPTH, 0, &clear_depth_value);
            
            glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),
                static_cast<float>(ui_w) / static_cast<float>(ui_h),
                0.1f,
                1000.0f
            );

            glm::mat4 view = glm::lookAt(
                glm::vec3{ 0.0f, 0.0f, 5.0f },
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

            shader_ui.uniform_mat4("u_model", glm::mat4{ 1.0f });
            mesh_coordinate_frame.draw(GL_LINES);

            shader_ui.uniform_mat4("u_model", glm::mat4{ 1.0f });
            shader_ui.uniform_bool("u_alpha", true);
            mesh_sphere.draw();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Render 3D objects to default framebuffer
        {
            glViewport(0, 0, window_w, window_h);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),
                static_cast<float>(window_w) / static_cast<float>(window_h),
                0.1f,
                1000.0f
            );

            shader_hopf.use();
            shader_hopf.uniform_float("u_time", glfwGetTime());
            shader_hopf.uniform_mat4("u_projection", projection);
            shader_hopf.uniform_mat4("u_view", arcball_camera_matrix);

            shader_hopf.uniform_mat4("u_model", arcball_model_matrix);
            hopf.draw(draw_as_points);

            if (show_floor_plane)
            {
                shader_hopf.uniform_mat4("u_model", glm::mat4{ 1.0f });
                mesh_grid.draw();
            }
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

