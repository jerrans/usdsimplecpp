#include "shader.h"
#include <fstream>

Shader::Shader()
{
	compiled = false;
	compile_failed = false;
}

void Shader::Set(std::string vertex_filename, std::string geometry_filename, std::string fragment_filename, std::vector<std::string> inputs, std::vector<std::string> outputs)
{
	vertex_path = vertex_filename;
	geometry_path = geometry_filename;
	fragment_path = fragment_filename;
	shader_inputs = inputs;
	shader_outputs = outputs;
	compiled = false;
	compile_failed = false;
}

void Shader::SetShaderSource(std::string vs_source, std::string gs_source, std::string fs_source, std::vector<std::string> inputs, std::vector<std::string> outputs)
{
	vertex_shader_source = vs_source;
	geometry_shader_source = gs_source;
	fragment_shader_source = fs_source;
	shader_inputs = inputs;
	shader_outputs = outputs;
	compiled = false;
	compile_failed = false;
}

Shader::~Shader()
{
	//
}

void Shader::AddMacro(std::string macro_name, std::string macro_value)
{
	std::string macro(" ");
	macro.append(macro_name);
	macro.append(" ");
	map_macro_value[macro] = macro_value;
}

bool Shader::CompileLink()
{
	if( vertex_path.length() > 0 )
		ReadShaderSource(vertex_path, vertex_shader_source);
	if( geometry_path.length() > 0 )
		ReadShaderSource(geometry_path, geometry_shader_source);
	if( fragment_path.length() > 0 )
		ReadShaderSource(fragment_path, fragment_shader_source);

	GLint isCompiled = 0;

	{
		vertex_program = glCreateShader(GL_VERTEX_SHADER);
		const GLchar *vertex_source = (GLchar *)vertex_shader_source.c_str();
		glShaderSource(vertex_program, 1, &vertex_source, 0);

		glCompileShader(vertex_program);

		glGetShaderiv(vertex_program, GL_COMPILE_STATUS, &isCompiled);
	
		GLint maxLength = 0;
		glGetShaderiv(vertex_program, GL_INFO_LOG_LENGTH, &maxLength);

		GLchar *infoLog = new GLchar[maxLength];
		glGetShaderInfoLog(vertex_program, maxLength, &maxLength, infoLog);

		if(isCompiled == GL_FALSE)
		{
			GL_LOG << vertex_path << std::endl;
			GL_LOG << infoLog << std::endl;

			// We don't need the shader anymore.
			glDeleteShader(vertex_program);

			delete[] infoLog;

			compile_failed = true;
			return false;
		}
	}

	if( geometry_shader_source.length() > 0 )
	{
		geometry_program = glCreateShader(GL_GEOMETRY_SHADER);
		const GLchar *geometry_source = (GLchar *)geometry_shader_source.c_str();
		glShaderSource(geometry_program, 1, &geometry_source, 0);

		glCompileShader(geometry_program);

		GLint isCompiled = 0;
		glGetShaderiv(geometry_program, GL_COMPILE_STATUS, &isCompiled);

		GLint maxLength = 0;
		glGetShaderiv(geometry_program, GL_INFO_LOG_LENGTH, &maxLength);

		GLchar *infoLog = new GLchar[maxLength];
		glGetShaderInfoLog(geometry_program, maxLength, &maxLength, infoLog);

		if(isCompiled == GL_FALSE)
		{
			GL_LOG << geometry_path << std::endl;
			GL_LOG << infoLog << std::endl;
			delete[] infoLog;
			// We don't need the shader anymore.
			glDeleteShader(geometry_program);
			glDeleteShader(vertex_program);

			compile_failed = true;
			return false;
		}
		delete[] infoLog;
	}

	{
		fragment_program = glCreateShader(GL_FRAGMENT_SHADER);

		const GLchar *fragment_source = (GLchar *)fragment_shader_source.c_str();
		glShaderSource(fragment_program, 1, &fragment_source, 0);

		glCompileShader(fragment_program);

		glGetShaderiv(fragment_program, GL_COMPILE_STATUS, &isCompiled);

		GLint maxLength = 0;
		glGetShaderiv(fragment_program, GL_INFO_LOG_LENGTH, &maxLength);

		GLchar *infoLog = new GLchar[maxLength];
		glGetShaderInfoLog(fragment_program, maxLength, &maxLength, infoLog);

		if(isCompiled == GL_FALSE)
		{
			GL_LOG << fragment_path << std::endl;
			GL_LOG << infoLog << std::endl;

			delete[] infoLog;

			glDeleteShader(fragment_program);
			glDeleteShader(vertex_program);
			if( geometry_shader_source.length() > 0 )
				glDeleteShader(geometry_program);

			compile_failed = true;
			return false;
		}
		delete[] infoLog;	
	}

	shader_program = glCreateProgram();

	glAttachShader(shader_program, vertex_program);
	if( geometry_shader_source.length() > 0 )
		glAttachShader(shader_program, geometry_program);
	glAttachShader(shader_program, fragment_program);

	GLsizei attach_count;
	GLuint attached_programs[3];
	glGetAttachedShaders(shader_program, 3, &attach_count, attached_programs);

	for( size_t i=0; i<shader_inputs.size(); ++i )
		glBindAttribLocation(shader_program, static_cast<GLuint>(i), shader_inputs[i].c_str());

	for( size_t i=0; i<shader_outputs.size(); ++i )
		glBindFragDataLocation(shader_program, static_cast<GLuint>(i), shader_outputs[i].c_str());

	glLinkProgram(shader_program);

	GLint isLinked = 0;
	glGetProgramiv(shader_program, GL_LINK_STATUS, (int *)&isLinked);
	if(isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		GLchar *infoLog = new GLchar[maxLength];
		glGetProgramInfoLog(shader_program, maxLength, &maxLength, infoLog);

		// We don't need the program anymore
		glDeleteProgram(shader_program);
		// Don't leak shaders either
		glDeleteShader(vertex_program);
		glDeleteShader(fragment_program);
		if( geometry_shader_source.length() > 0 )
			glDeleteShader(geometry_program);

		GL_LOG << infoLog << std::endl;

		delete[] infoLog;

		compile_failed = true;
		return false;
	}

	// Always detach shaders after a successful link.
	glDetachShader(shader_program, vertex_program);
	glDetachShader(shader_program, fragment_program);
	if( geometry_shader_source.length() > 0 )
		glDetachShader(shader_program, geometry_program);

	compiled = true;
	return true;
}

std::string Shader::ProcessMacros(std::string &line)
{
	size_t defpos = line.find("#define");
	if( defpos == std::string::npos )
		return line;

	size_t macropos = std::string::npos;
	for( std::map<std::string, std::string>::iterator it = map_macro_value.begin(); it != map_macro_value.end(); ++it )
	{
		macropos = line.find(it->first);
		if( macropos != std::string::npos )
		{
			return "#define " + it->first + " " + it->second;
		}
	}

	return line;
}

void Shader::ReadShaderSource(std::string file_path, std::string &shader)
{
	std::vector<std::string> text;
	std::string line;
	std::ifstream textstream (file_path);
	if( textstream.is_open() )
	{
		while (getline(textstream, line)) {
			text.push_back(ProcessMacros(line) + "\n");
		} 
		textstream.close();
		for (size_t i=0; i < text.size(); i++)
			shader += text[i];
	}else
		GL_LOG << "Failed to open file: " << file_path << std::endl;
}