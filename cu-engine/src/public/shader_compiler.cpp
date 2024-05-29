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

bool compile_spirv(const std::string& p_filepath, const std::string& p_out_path, std::vector<unsigned int>& p_spirv) {
	std::ifstream file(p_filepath);
	if (!file.is_open()) {
		ENGINE_ERROR("failed to open file {}!", p_filepath);
		return false;
	}
	std::string input_glsl((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	glslang::TShader shader(EShLangVertex);
	const char *input_string = input_glsl.c_str();
	shader.setStrings(&input_string, 1);
	int client_input_semantics_version = 100; // maps to, say, #define VULKAN 100
	glslang::EShTargetClientVersion vulkan_client_version = glslang::EShTargetVulkan_1_0;
	glslang::EShTargetLanguageVersion target_version = glslang::EShTargetSpv_1_0;

	shader.setEnvInput(glslang::EShSourceGlsl, EShLangVertex, glslang::EShClientVulkan, client_input_semantics_version);
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
	if (!shader.parse(resources, 100, false, messages))
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
	glslang::GlslangToSpv(*Program.getIntermediate(EShLangVertex), p_spirv, &logger, &spv_options);
	glslang::OutputSpvBin(p_spirv, p_out_path.c_str());
	return true;
}


bool ShaderCompiler::compile_shader(const std::string& p_filepath, CompiledShaderInfo *out_info) {
	const std::string out_path = "spv_" + std::filesystem::path(p_filepath).filename().string();
	const bool in_file_exists = std::filesystem::exists(std::filesystem::path(p_filepath));
	const bool out_file_exists = std::filesystem::exists(std::filesystem::path(out_path));
	if (out_file_exists && in_file_exists) {
		
		std::filesystem::file_time_type time1 = std::filesystem::last_write_time(std::filesystem::path(p_filepath));
		std::filesystem::file_time_type time2 = std::filesystem::last_write_time(std::filesystem::path(out_path));

		if (time1 > time2) {
			std::vector<unsigned int> spirv;
			if (compile_spirv(p_filepath, out_path, spirv)) {
				if (out_info) {
					out_info->code = spirv.data();
					out_info->code_size = spirv.size() * sizeof(uint32_t);
				}
			}
		}
	} else if (out_file_exists) {
		std::ifstream out_file(out_path, std::ios::ate | std::ios::binary);
		size_t file_size = (size_t) out_file.tellg();
		std::vector<char> code(file_size);
		out_file.seekg(0);
		out_file.read(code.data(), code.size());
		out_file.close();

		out_info->code = reinterpret_cast<const uint32_t*>(code.data());
		out_info->code_size = code.size();
	} else if (in_file_exists) {
		std::vector<unsigned int> spirv;
		if (compile_spirv(p_filepath, out_path, spirv)) {
			if (out_info) {
				out_info->code = spirv.data();
				out_info->code_size = spirv.size() * sizeof(uint32_t);
			}
		}
	} else {
		ENGINE_ERROR("Failed to compile: {}", p_filepath);
		return false;
	}
    return true;
}