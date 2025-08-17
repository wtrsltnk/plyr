#ifndef GLPROGRAM_HPP
#define GLPROGRAM_HPP

#include "glshader.hpp"
#include <glm/gtc/type_ptr.hpp>

class GlProgram
{
public:
    GlProgram()
    {
        _index = glCreateProgram();
    }

    ~GlProgram()
    {
        if (is_good())
        {
            glDeleteProgram(_index);
            _index = 0;
        }
    }

    template <int Type>
    void attach(const GlShader<Type> &shader)
    {
        glAttachShader(_index, shader._index);
    }

    void link()
    {
        glLinkProgram(_index);

        GLint result = GL_FALSE;
        GLint logLength;

        glGetProgramiv(_index, GL_LINK_STATUS, &result);
        if (result == GL_FALSE)
        {
            glGetProgramiv(_index, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<char> error(static_cast<size_t>((logLength > 1) ? logLength : 1));
            glGetProgramInfoLog(_index, logLength, NULL, &error[0]);

            std::cerr << "tried linking program, got error:\n"
                      << error.data();
        }
    }

    GLint getAttribLocation(const char *name) const { return glGetAttribLocation(_index, name); }

    void setUniformMatrix(const char *name, const glm::mat4 &m)
    {
        unsigned int model = glGetUniformLocation(_index, name);
        glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(m));
    }

    void use() const { glUseProgram(_index); }

    bool is_good() const { return _index > 0; }

private:
    GLuint _index = 0;
};

#endif // GLPROGRAM_HPP
