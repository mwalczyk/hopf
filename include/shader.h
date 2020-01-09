#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "glad/glad.h"

class Shader
{
public:

    Shader(const std::string& vert_path, const std::string& frag_path)
    {
        // Load the shader modules
        uint32_t vert = compile_shader_module(vert_path, GL_VERTEX_SHADER);
        uint32_t frag = compile_shader_module(frag_path, GL_FRAGMENT_SHADER);

        // Create the shader program
        program_id = glCreateProgram();
        glAttachShader(program_id, vert);
        glAttachShader(program_id, frag);
        glLinkProgram(program_id);
        check_compilation_errors(program_id, "program");

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    ~Shader()
    {
        glDeleteProgram(program_id);
    }

    uint32_t get_handle() const
    {
        return program_id;
    }

    void use() const
    {
        glUseProgram(program_id);
    }

    void uniform_bool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(program_id, name.c_str()), (int)value);
    }

    void uniform_int(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(program_id, name.c_str()), value);
    }

    void uniform_float(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(program_id, name.c_str()), value);
    }

    void uniform_vec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(program_id, name.c_str()), 1, &value[0]);
    }
    void uniform_vec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(program_id, name.c_str()), x, y);
    }

    void uniform_vec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(program_id, name.c_str()), 1, &value[0]);
    }
    void uniform_vec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(program_id, name.c_str()), x, y, z);
    }

    void uniform_vec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(program_id, name.c_str()), 1, &value[0]);
    }
    void uniform_vec4(const std::string& name, float x, float y, float z, float w)
    {
        glUniform4f(glGetUniformLocation(program_id, name.c_str()), x, y, z, w);
    }

    void uniform_mat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(program_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void uniform_mat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(program_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void uniform_mat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(program_id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:

    uint32_t program_id;

    uint32_t compile_shader_module(const std::string& path, uint32_t type)
    {
        std::string code;
        std::ifstream file;

        // Ensure that `ifstream` objects can throw exceptions
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            file.open(path);
            std::stringstream shader_stream;

            // Read file's buffer contents into a stream
            shader_stream << file.rdbuf();
            file.close();

            // Convert stream into string
            code = shader_stream.str();
        }
        catch (std::ifstream::failure e)
        {
            std::cerr << "Shader file not successfully read\n";
        }

        const char* shader_code = code.c_str();

        uint32_t shader_module = glCreateShader(type);
        glShaderSource(shader_module, 1, &shader_code, NULL);
        glCompileShader(shader_module);
        check_compilation_errors(shader_module, type == GL_VERTEX_SHADER ? "vertex" : "fragment");

        return shader_module;
    }

    void check_compilation_errors(uint32_t shader, const std::string& type)
    {
        int success;
        char info[1024];

        if (type != "program")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, info);
                std::cout << "[Shader Compilation Error] of type: " << type << "\n" << info << "\n";
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);

            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, info);
                std::cout << "[Program Linking Error] of type: " << type << "\n" << info << "\n";
            }
        }
    }

    void perform_reflection()
    {
        struct uniform_info_t
        {
            GLint location;
            GLsizei count;
        };

        GLint uniform_count = 0;
        glGetProgramiv(program_id, GL_ACTIVE_UNIFORMS, &uniform_count);

        if (uniform_count != 0)
        {
            GLint 	max_name_len = 0;
            GLsizei length = 0;
            GLsizei count = 0;
            GLenum 	type = GL_NONE;
            glGetProgramiv(program_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_len);

            auto uniform_name = std::make_unique<char[]>(max_name_len);

            std::unordered_map<std::string, uniform_info_t> uniforms;

            for (GLint i = 0; i < uniform_count; ++i)
            {
                glGetActiveUniform(program_id, i, max_name_len, &length, &count, &type, uniform_name.get());

                uniform_info_t uniform_info = {};
                uniform_info.location = glGetUniformLocation(program_id, uniform_name.get());
                uniform_info.count = count;

                uniforms.emplace(std::make_pair(std::string(uniform_name.get(), length), uniform_info));
            }
        }
    }
};