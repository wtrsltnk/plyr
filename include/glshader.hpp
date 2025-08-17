#ifndef GLSHADER_HPP
#define GLSHADER_HPP

#include <entt/entt.hpp>
#include <glad/glad.h>
#include <iostream>
#include <vector>

#define GLSL(src) "#version 330 core\n" #src
#define GLSL_VERTEX_SHADER(src) GlShader<GL_VERTEX_SHADER>(GLSL(src))
#define GLSL_FRAGMENT_SHADER(src) GlShader<GL_FRAGMENT_SHADER>(GLSL(src))

template <int Type>
class GlShader
{
public:
    GlShader(const char *source)
    {
        _index = glCreateShader(Type);

        glShaderSource(_index, 1, &source, NULL);
        glCompileShader(_index);

        GLint result = GL_FALSE;
        GLint logLength;

        glGetShaderiv(_index, GL_COMPILE_STATUS, &result);
        if (result == GL_FALSE)
        {
            glGetShaderiv(_index, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> error(static_cast<size_t>((logLength > 1) ? logLength : 1));
            glGetShaderInfoLog(_index, logLength, NULL, &error[0]);

            std::cerr << "tried compiling shader\n\t" << source << "\n\n"
                      << "got error\n\t" << error.data() << std::endl;

            glDeleteShader(_index);

            _index = 0;
        }
    }

    ~GlShader()
    {
        if (is_good())
        {
            glDeleteShader(_index);

            _index = 0;
        }
    }

    bool is_good() const { return _index > 0; }

private:
    GLuint _index = 0;

    friend class GlProgram;
};

#endif // GLSHADER_HPP
