#include "shader_compiler.h"

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>

#include <fstream>

#include "logger.h"
#include <filesystem>


ShaderCompiler *ShaderCompiler::singleton = nullptr;

ShaderCompiler::ShaderCompiler() {
    singleton = this;
    glslang::InitializeProcess();
}

ShaderCompiler::~ShaderCompiler() {
    glslang::FinalizeProcess();
    singleton = nullptr;
}

ShaderCompiler *ShaderCompiler::get_singleton() {
    return singleton;
}

std::string get_file_path(const std::string& str)
{
    size_t found = str.find_last_of("/\\");
    return str.substr(0,found);
    //size_t FileName = str.substr(found+1);
}

std::string get_file_extension(const std::string& name)
{
    const size_t pos = name.rfind('.');
    return (pos == std::string::npos) ? "" : name.substr(name.rfind('.') + 1);
}

bool get_shader_stage(const std::string& p_file_extension, EShLanguage& p_out_stage) {
	const size_t pos = p_file_extension.rfind('.');
	const std::string stage = (pos == std::string::npos) ? "" : p_file_extension.substr(p_file_extension.rfind('.') + 1);
	if (stage == "vert") {
		p_out_stage = EShLangVertex;
	} else if (stage == "tesc") {
        p_out_stage = EShLangTessControl;
    } else if (stage == "tese") {
        p_out_stage = EShLangTessEvaluation;
    } else if (stage == "geom") {
        p_out_stage = EShLangGeometry;
    } else if (stage == "frag") {
        p_out_stage = EShLangFragment;
    } else if (stage == "comp") {
        p_out_stage = EShLangCompute;
    } else {
        return false;
    }
	return true;
}

bool get_shader_stage(const std::string& p_file_extension, ShaderStage& p_out_stage) {
	const size_t pos = p_file_extension.rfind('.');
	const std::string stage = (pos == std::string::npos) ? "" : p_file_extension.substr(p_file_extension.rfind('.') + 1);
	if (stage == "vert") {
		p_out_stage = VERTEX;
	} else if (stage == "tesc") {
        //p_out_stage = EShLangTessControl;
		return false;
    } else if (stage == "tese") {
        //p_out_stage = EShLangTessEvaluation;
		return false;
    } else if (stage == "geom") {
        //p_out_stage = EShLangGeometry;
		return false;
    } else if (stage == "frag") {
        p_out_stage = FRAGMENT;
    } else if (stage == "comp") {
        //p_out_stage = EShLangCompute;
		return false;
    } else {
        return false;
    }
	return true;
}

bool compile_spirv(const std::string& p_filepath, const std::string& p_out_path, std::vector<unsigned int>& p_spirv) {
	std::ifstream file(p_filepath);
	if (!file.is_open()) {
		ENGINE_ERROR("failed to open file {}!", p_filepath);
		return false;
	}

	EShLanguage shader_stage;
	if (!get_shader_stage(p_filepath, shader_stage)) {
		ENGINE_ERROR("No shader stage found for {}", p_filepath);
		return false;
	}

	std::string input_glsl((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	glslang::TShader shader(shader_stage);
	const char *input_string = input_glsl.c_str();
	shader.setStrings(&input_string, 1);
	int client_input_semantics_version = 100; // maps to, say, #define VULKAN 100
	glslang::EShTargetClientVersion vulkan_client_version = glslang::EShTargetVulkan_1_0;
	glslang::EShTargetLanguageVersion target_version = glslang::EShTargetSpv_1_0;

	shader.setEnvInput(glslang::EShSourceGlsl, shader_stage, glslang::EShClientVulkan, client_input_semantics_version);
	shader.setEnvClient(glslang::EShClientVulkan, vulkan_client_version);
	shader.setEnvTarget(glslang::EShTargetSpv, target_version);

	const TBuiltInResource *resources = GetDefaultResources();
	EShMessages messages = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);

	const int default_version = 100;

	DirStackFileIncluder includer;

	std::string path = get_file_path(p_filepath);
	includer.pushExternalLocalDirectory(path);

	std::string preprocess_glsl;

	if (!shader.preprocess(resources, default_version, ENoProfile, false, false, messages, &preprocess_glsl, includer)) {
		ENGINE_ERROR("GLSL Preprocessing Failed for: {}", p_filepath);
		ENGINE_ERROR("",shader.getInfoLog());
		ENGINE_ERROR("",shader.getInfoDebugLog());
		return false;
	}
	const char* preprocess_cstr = preprocess_glsl.c_str();
	shader.setStrings(&preprocess_cstr, 1);
	if (!shader.parse(resources, default_version, false, messages))
	{
		ENGINE_ERROR("GLSL Parsing Failed for: {}",p_filepath);
		ENGINE_ERROR("",shader.getInfoLog());
		ENGINE_ERROR("",shader.getInfoDebugLog());
		return false;
	}
	glslang::TProgram Program;
	Program.addShader(&shader);

	if(!Program.link(messages))
	{
		ENGINE_ERROR("GLSL Linking Failed for: {0} ", p_filepath);
		ENGINE_ERROR("",shader.getInfoLog());
		ENGINE_ERROR("",shader.getInfoDebugLog());
		return false;
	}
	spv::SpvBuildLogger logger;
	glslang::SpvOptions spv_options;
	glslang::GlslangToSpv(*Program.getIntermediate(shader_stage), p_spirv, &logger, &spv_options);
	glslang::OutputSpvBin(p_spirv, p_out_path.c_str());
	return true;
}


bool ShaderCompiler::compile_shader(const std::string& p_filepath, CompiledShaderInfo *out_info) {
	const std::string out_path = std::filesystem::path(get_file_path(p_filepath)) / std::filesystem::path("spv_" + std::filesystem::path(p_filepath).filename().string());
	const bool in_file_exists = std::filesystem::exists(std::filesystem::path(p_filepath));
	const bool out_file_exists = std::filesystem::exists(std::filesystem::path(out_path));
	ShaderStage shader_stage;
	if (!get_shader_stage(p_filepath, shader_stage)) {
		ENGINE_ERROR("No shader stage found for {}", p_filepath);
		return false;
	}
	if (out_file_exists && in_file_exists) {
		
		std::filesystem::file_time_type time1 = std::filesystem::last_write_time(std::filesystem::path(p_filepath));
		std::filesystem::file_time_type time2 = std::filesystem::last_write_time(std::filesystem::path(out_path));

		if (time1 > time2) {
			std::vector<unsigned int> spirv;
			if (compile_spirv(p_filepath, out_path, spirv)) {
				if (out_info) {
					out_info->buffer = spirv;
					out_info->stage = shader_stage;
				}
			}
		} else {
			std::ifstream out_file(out_path, std::ios::ate | std::ios::binary);
			if (!out_file.is_open()) {
				return false;
			}
			size_t file_size = (size_t)out_file.tellg();
			std::vector<uint32_t> buffer(file_size / sizeof(u_int32_t));
			out_file.seekg(0);
			out_file.read((char*)buffer.data(), file_size);
			out_file.close();

			out_info->buffer = buffer;
			out_info->stage = shader_stage;
		}
	} else if (in_file_exists && !out_file_exists) {
		std::vector<unsigned int> spirv;
		if (compile_spirv(p_filepath, out_path, spirv)) {
			if (out_info) {
				out_info->buffer = spirv;
				out_info->stage = shader_stage;
			}
		}
	} else {
		ENGINE_ERROR("Failed to compile: {}", p_filepath);
		return false;
	}
    return true;
}