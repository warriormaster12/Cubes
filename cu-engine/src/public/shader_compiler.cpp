#include "shader_compiler.h"

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <StandAlone/DirStackFileIncluder.h>

#include <fstream>

#include "logger.h"


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


bool ShaderCompiler::compile_shader(const std::string& p_file, const ShaderStage stage, CompiledShaderInfo *out_info) {

	std::ifstream file(p_file);

    if (!file.is_open()) {
        ENGINE_ERROR("failed to open file {}!", p_file);
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

	std::string path = get_file_path(p_file);
	includer.pushExternalLocalDirectory(path);

	std::string preprocess_glsl;

	if (!shader.preprocess(resources, default_version, ENoProfile, false, false, messages, &preprocess_glsl, includer)) {
		ENGINE_ERROR("GLSL Preprocessing Failed for: {}", p_file);
		ENGINE_ERROR("",shader.getInfoLog());
		ENGINE_ERROR("",shader.getInfoDebugLog());
		return false;
	}
	const char* preprocess_cstr = preprocess_glsl.c_str();
	shader.setStrings(&preprocess_cstr, 1);
	if (!shader.parse(resources, 100, false, messages))
	{
		ENGINE_ERROR("GLSL Parsing Failed for: {}",p_file);
		ENGINE_ERROR("",shader.getInfoLog());
		ENGINE_ERROR("",shader.getInfoDebugLog());
		return false;
	}
	glslang::TProgram Program;
	Program.addShader(&shader);

	if(!Program.link(messages))
	{
		ENGINE_ERROR("GLSL Linking Failed for: {0} ", p_file);
		ENGINE_ERROR("",shader.getInfoLog());
		ENGINE_ERROR("",shader.getInfoDebugLog());
		return false;
	}
	std::vector<unsigned int> spirv;
	spv::SpvBuildLogger logger;
	glslang::SpvOptions spv_options;
	glslang::GlslangToSpv(*Program.getIntermediate(EShLangVertex), spirv, &logger, &spv_options);
	glslang::OutputSpvBin(spirv, "test.spv");
	if (out_info) {
		out_info->code = spirv.data();
		out_info->code_size = spirv.size() * sizeof(uint32_t);
	}
    return true;
}