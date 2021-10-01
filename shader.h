#include <string>
#include <vector>
#include <map>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
//#define GLEW_STATIC
#include <GL/glew.h>

#define GL_LOG std::cout

class Shader
{
public:
	Shader();
	Shader(std::string vertex_path, std::string geometry_path, std::string fragment_path, std::vector<std::string> inputs, std::vector<std::string> outputs);
	virtual ~Shader();
	void Set(std::string vertex_filename, std::string geometry_filename, std::string fragment_filename, std::vector<std::string> inputs, std::vector<std::string> outputs);
	void SetShaderSource(std::string vertex_shader_source, std::string geometry_shader_source, std::string fragment_shader_source, std::vector<std::string> inputs, std::vector<std::string> outputs);
	void AddMacro(std::string macro_name, std::string macro_value);
	bool CompileLink();
	bool Compiled() { return compiled; }
	bool CompileFailed() { return compile_failed; }
	GLuint Program(){ return shader_program; }
	inline void Activate()
	{
		if( !compiled )
		{
			GL_LOG << "Shader::Activate Attempted to activate a shader that has not been compiled." << std::endl;
		}
		glUseProgram(shader_program);
	}
	inline void Deactivate(){ glUseProgram(0); }
	// some helper functions
	inline void SetUniform(const char *name, int val){ glUniform1i(glGetUniformLocation(shader_program, name), val); }
	inline void SetUniform(const char *name, float val){ glUniform1f(glGetUniformLocation(shader_program, name), val); }
	inline void SetUniform(const char *name, glm::mat4 &val){ glUniformMatrix4fv(glGetUniformLocation(shader_program, name), 1, false, glm::value_ptr(val)); }
	inline void SetUniform(const char *name, glm::vec2 &val){ glUniform2f(glGetUniformLocation(shader_program, name), val[0], val[1]); }
	inline void SetUniform(const char *name, glm::vec3 &val){ glUniform3f(glGetUniformLocation(shader_program, name), val[0], val[1], val[2]); }
	inline void SetUniform(const char *name, glm::vec4 &val){ glUniform4f(glGetUniformLocation(shader_program, name), val[0], val[1], val[2], val[3]); }
	inline void SetUniform(const char *name, GLuint64EXT val){ glUniformui64NV(glGetUniformLocation(shader_program, name), val); }

protected:
	void ReadShaderSource(std::string file_path, std::string &shader);
	std::string ProcessMacros(std::string &line);
	int shader_type;
	bool compiled;
	bool compile_failed;
	std::vector<std::string> shader_inputs, shader_outputs;
	std::map<std::string, std::string> map_macro_value;
	std::string vertex_path, geometry_path, fragment_path;
	std::string vertex_shader_source, geometry_shader_source, fragment_shader_source;
	GLuint vertex_program, geometry_program, fragment_program, shader_program;
};
