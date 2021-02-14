#pragma once

#include <mutex>
#include <condition_variable>
#include <random>
#include "vector.hpp"
#include "Vk.hpp"
#include "Map.hpp"
#include "math.hpp"
#include "Pipeline.hpp"
#include "Material.hpp"
#include "Model.hpp"
#include "Pool.hpp"
#include <GLFW/glfw3.h>

namespace Rosee {

using PipelinePool = Pool<Pipeline>;
using MaterialPool = Pool<Material>;
using ModelPool = Pool<Model>;

struct AccelerationStructure : public Vk::Handle<VkAccelerationStructureKHR>
{
public:
	AccelerationStructure(void) = default;

	using Vk::Handle<VkAccelerationStructureKHR>::operator=;

	Vk::BufferAllocation buffer;
	VkDeviceAddress reference;

	// to store acceleration-specific indices, must be filled by user and indexType set to let destroy() take care of that
	VkIndexType indexType = VK_INDEX_TYPE_NONE_KHR;
	Vk::BufferAllocation indexBuffer;
};

using AccelerationStructurePool = Pool<AccelerationStructure>;

struct Camera {
	glm::dmat4 last_view;
	glm::dmat4 view;
	glm::dmat4 proj;
	float near;
	float far;
	glm::vec2 ratio;
};

namespace Vertex {

struct p2
{
	glm::vec2 p;
};

struct pn
{
	glm::vec3 p;
	//float _pad0;
	glm::vec3 n;
};

struct pnu
{
	glm::vec3 p;
	//float _pad0;
	glm::vec3 n;
	//float _pad1;
	glm::vec2 u;
	//glm::vec2 _pad2;
};

struct pntbu
{
	glm::vec3 p;
	glm::vec3 n;
	glm::vec3 t;
	glm::vec3 b;
	glm::vec2 u;
};

}

struct Material_albedo {
	uint32_t albedo;
};

struct CustomInstance {
	glm::mat3 mv_normal;
	uint32_t model;
	uint32_t material;
};

class Renderer
{
	uint32_t m_frame_count;
	bool m_validate;
	bool m_use_render_doc;

	void throwGlfwError(void);
	glm::ivec2 m_window_last_pos;
	glm::ivec2 m_window_last_size;
	bool m_fullscreen = false;

	GLFWwindow *m_window;
	GLFWwindow *createWindow(void);
	svec2 getWindowSize(void) const;

	uint32_t m_instance_version;
	Vk::Instance m_instance;
	Vk::Instance createInstance(void);
	Vk::DebugUtilsMessengerEXT m_debug_messenger;
	Vk::DebugUtilsMessengerEXT createDebugMessenger(void);
	Vk::SurfaceKHR m_surface;
	Vk::SurfaceKHR createSurface(void);
	VkPhysicalDeviceProperties m_properties;
	VkPhysicalDeviceLimits m_limits;
	VkPhysicalDeviceFeatures m_features;
	VkPresentModeKHR m_present_mode;
	VkSurfaceFormatKHR m_surface_format;
	VkSurfaceCapabilitiesKHR m_surface_capabilities;
	uint32_t m_queue_family_graphics = ~0U;
	uint32_t m_queue_family_compute = ~0U;
	VkPhysicalDevice m_physical_device;

	struct Ext {
		bool ray_tracing;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_props;
	};
public:
	Ext ext;
	VkSharingMode m_gc_sharing_mode;
	uint32_t m_gc_unique_count;
	uint32_t m_gc_uniques[2];
	Vk::Device device;

private:
	Vk::Device createDevice(void);

public:
	const VkFormat format_depth;

private:
	VkFormat getFormatDepth(void);
	const VkSampleCountFlags m_supported_sample_counts;
	VkSampleCountFlags getSupportedSampleCounts(void);
	VkSampleCountFlagBits fitSampleCount(VkSampleCountFlagBits sampleCount) const;

private:
	Vk::PipelineCache m_pipeline_cache;

public:
	Vk::Allocator allocator;

private:
	Vk::Allocator createAllocator(void);

	Vk::Queue m_gqueue;
	Vk::Queue m_cqueue;

public:
	void waitIdle(void)
	{
		m_gqueue.waitIdle();
		if (m_cqueue != VK_NULL_HANDLE)
			m_cqueue.waitIdle();
	}

private:
	VkExtent2D m_swapchain_extent;
	uint32_t m_swapchain_mip_levels;
	VkExtent2D m_swapchain_extent_mip;
	vector<VkExtent2D> m_swapchain_extent_mips;
	static uint32_t extentLog2(uint32_t val);
	static uint32_t extentMipLevels(const VkExtent2D &extent);
	static uint32_t nextExtentMip(uint32_t extent);
	static uint32_t divAlignUp(uint32_t a, uint32_t b);

public:
	const VkExtent2D& swapchainExtent(void) const { return m_swapchain_extent; }

private:
	struct PipelineViewportState {
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo ci;
	} m_pipeline_viewport_state;
	Vk::SwapchainKHR m_swapchain;
	Vk::SwapchainKHR createSwapchain(void);
	vector<VkImage> m_swapchain_images;
	vector<Vk::ImageView> m_swapchain_image_views;

public:
	Vk::ImageView createImageView(VkImage image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspect);
	Vk::ImageView createImageViewMip(VkImage image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspect,
		uint32_t baseMipLevel, uint32_t levelCount);

private:
	vector<Vk::ImageView> createSwapchainImageViews(void);

	Vk::CommandPool m_gcommand_pool;
	Vk::CommandPool m_ccommand_pool;
	Vk::CommandPool m_transfer_command_pool;
	Vk::CommandBuffer m_transfer_cmd;
	Vk::CommandPool m_ctransfer_command_pool;
	Vk::CommandBuffer m_ctransfer_cmd;

public:
	static inline constexpr uint32_t s0_sampler_count = 256;
	static inline constexpr uint32_t modelPoolSize = 512;
	static inline constexpr uint32_t materialPoolSize = 256;

private:
	static inline constexpr uint32_t const_sets_per_frame = 4;
	static inline constexpr uint32_t sets_per_frame_mip = 1;

	Vk::DescriptorSetLayout m_descriptor_set_layout_0;
	Vk::DescriptorSetLayout createDescriptorSetLayout0(void);
	Vk::DescriptorSetLayout m_descriptor_set_layout_dynamic;
	Vk::DescriptorSetLayout createDescriptorSetLayoutDynamic(void);
	Vk::PipelineLayout m_pipeline_layout_descriptor_set;
	Vk::PipelineLayout createPipelineLayoutDescriptorSet(void);
	Vk::ShaderModule m_fwd_p2_module;
	VkSampleCountFlagBits m_sample_count;

public:
	struct IllumTechnique {
		struct Data {
			struct Potato {
				static inline constexpr uint32_t descriptorCombinedImageSamplerCount = 3;
				static inline constexpr uint32_t barrsPerFrame = 0;
				static inline constexpr uint32_t addBarrsPerFrame = 0;
			};

			struct Sspt {
				static inline constexpr uint32_t descriptorCombinedImageSamplerCount = 1 +	// depth_resolve
					6 +
					7;
				static inline constexpr uint32_t msDescriptorCombinedImageSamplerCount = 1 +	// depth_resolve
					3 +	// color_resolve
					9 +
					7;
				static inline constexpr uint32_t barrsPerFrame = 4;
				static inline constexpr uint32_t msBarrsPerFrame = 4 + 2;
				static inline constexpr uint32_t addBarrsPerFrame = 7;
				static inline constexpr uint32_t msAddBarrsPerFrame = 7 + 2;

				struct Fbs {
					Vk::ImageAllocation m_albedo_resolved;
					Vk::ImageView m_albedo_resolved_view;
					Vk::ImageAllocation m_normal_resolved;
					Vk::ImageView m_normal_resolved_view;
					Vk::Framebuffer m_color_resolve_fb;
					Vk::Framebuffer createColorResolveFb(Renderer &r);
					VkDescriptorSet m_color_resolve_set;

					Vk::ImageAllocation m_depth;
					Vk::ImageView m_depth_view;
					Vk::ImageView m_depth_first_mip_view;

					Vk::Framebuffer m_depth_resolve_fb;
					Vk::Framebuffer createDepthResolveFb(Renderer &r);
					VkDescriptorSet m_depth_resolve_set;
					vector<Vk::ImageView> m_depth_acc_views;
					vector<Vk::ImageView> createDepthAccViews(Renderer &r);
					vector<Vk::Framebuffer> m_depth_acc_fbs;
					vector<Vk::Framebuffer> createDepthAccFbs(Renderer &r);
					vector<VkDescriptorSet> m_depth_acc_sets;
					vector<VkDescriptorSet> createDepthAccSets(Renderer &r, const VkDescriptorSet *pDescriptorSetsMip);

					Vk::ImageAllocation m_step;
					Vk::ImageView m_step_view;
					Vk::ImageAllocation m_acc_path_pos;
					Vk::ImageView m_acc_path_pos_view;
					Vk::ImageAllocation m_direct_light;
					Vk::ImageView m_direct_light_view;
					Vk::ImageAllocation m_path_albedo;
					Vk::ImageView m_path_albedo_view;
					Vk::ImageAllocation m_path_direct_light;
					Vk::ImageView m_path_direct_light_view;
					Vk::ImageAllocation m_path_incidence;
					Vk::ImageView m_path_incidence_view;

					void destroy(Renderer &r);
				};
			};

			struct RayTracing {
				static inline constexpr uint32_t groupCount = 5;
				static inline constexpr uint32_t bufWritesPerFrame = 2;
				static inline constexpr uint32_t customInstancePoolSize = 16000000;

				struct Shared {
					VkDescriptorSetLayout m_res_set_layout;
					VkDescriptorSetLayout createResSetLayout(Renderer &r);

					Pipeline m_pipeline;
					Pipeline createPipeline(Renderer &r);

					Vk::BufferAllocation m_sbt_raygen_buffer;
					Vk::BufferAllocation m_sbt_miss_buffer;
					Vk::BufferAllocation m_sbt_hit_buffer;
					VkStridedDeviceAddressRegionKHR m_sbt_raygen_region;
					VkStridedDeviceAddressRegionKHR m_sbt_miss_region;
					VkStridedDeviceAddressRegionKHR m_sbt_hit_region;
					VkStridedDeviceAddressRegionKHR m_sbt_callable_region;

					void destroy(Renderer &r);
				};

				struct Fbs {
					uint32_t instance_count;
					void *m_instance_buffer_staging_ptr;
					Vk::BufferAllocation m_instance_buffer_staging;
					Vk::BufferAllocation m_instance_buffer;
					VkDeviceAddress m_instance_addr;
					Vk::BufferAllocation m_scratch_buffer;
					VkDeviceAddress m_scratch_addr;

					AccelerationStructure m_top_acc_structure;

					void *m_illumination_staging_ptr;
					Vk::BufferAllocation m_illumination_staging;

					VkDescriptorSet m_res_set;
					Vk::BufferAllocation m_materials_albedo_buffer;
					Vk::BufferAllocation m_custom_instance_buffer;
					void *m_custom_instance_buffer_staging_ptr;
					Vk::BufferAllocation m_custom_instance_buffer_staging;

					void destroy(Renderer &r);
					void destroy_acc(Renderer &r);	// destroy only acc structure & related buffers
				};
			};

			struct Rtpt {
				static inline constexpr uint32_t storageImageCount = 9;
				static inline constexpr uint32_t descriptorCombinedImageSamplerCount = 6 + 9;
				static inline constexpr uint32_t barrsPerFrame = 3;
				static inline constexpr uint32_t addBarrsPerFrame = 9;

				struct Fbs {
					Vk::ImageAllocation m_step;
					Vk::ImageView m_step_view;
					Vk::ImageAllocation m_acc;
					Vk::ImageView m_acc_view;
					Vk::ImageAllocation m_path_next_origin;
					Vk::ImageView m_path_next_origin_view;
					Vk::ImageAllocation m_path_next_direction;
					Vk::ImageView m_path_next_direction_view;
					Vk::ImageAllocation m_path_albedo;
					Vk::ImageView m_path_albedo_view;
					Vk::ImageAllocation m_path_normal;
					Vk::ImageView m_path_normal_view;
					Vk::ImageAllocation m_path_direct_light;
					Vk::ImageView m_path_direct_light_view;
					Vk::ImageAllocation m_direct_light;
					Vk::ImageView m_direct_light_view;

					void destroy(Renderer &r);
				};
			};

			struct Rtdp {
				static inline constexpr uint32_t storageImageCount = 3;

				static inline constexpr uint32_t descriptorCombinedImageSamplerCount = 5;
				static inline constexpr uint32_t barrsPerFrame = 0;
				static inline constexpr uint32_t addBarrsPerFrame = 0;

				static inline constexpr uint32_t bufWritesPerFrame = 1;	// probes_pos


				static inline constexpr uint32_t probeLayerCount = 3;
				static inline constexpr uint32_t probeSizeL2 = 3;
				static inline constexpr uint32_t probeDiffuseSize = 8;
				static inline constexpr uint32_t probeMaxBounces = 4;

				struct Probe {
					glm::vec4 pos;
					glm::vec2 ipos;
					glm::vec2 depth;
				};

				struct Shared {
					Pipeline m_schedule_pipeline;
					Pipeline createSchedulePipeline(Renderer &r);
					Pipeline m_pipeline;
					Pipeline createPipeline(Renderer &r);
					Pipeline m_diffuse_pipeline;
					Pipeline createDiffusePipeline(Renderer &r);

					glm::vec3 m_rnd_sun[8192];
					glm::vec3 m_rnd_diffuse[8192];

					void destroy(Renderer &r);
				};

				struct Fbs {
					glm::ivec2 m_probe_extent;
					uint32_t m_probe_size_l2;
					uint32_t m_probe_size;
					Vk::BufferAllocation m_probes_pos;
					Vk::BufferAllocation createProbesPos(Renderer &r);
					Vk::ImageAllocation m_probes;
					Vk::ImageView m_probes_view;
					Vk::ImageAllocation m_probes_diffuse;
					Vk::ImageView m_probes_diffuse_view;

					void destroy(Renderer &r);
				};
			};

			struct Rtbp {
				static inline constexpr uint32_t storageImageCount = 5;

				static inline constexpr uint32_t descriptorCombinedImageSamplerCount = 12;
				static inline constexpr uint32_t barrsPerFrame = 5;
				static inline constexpr uint32_t addBarrsPerFrame = 5;

				static inline constexpr uint32_t bufWritesPerFrame = 0;

				struct Shared {
					// nothing yet !

					void destroy(Renderer &r);
				};

				struct Fbs {
					Vk::ImageAllocation m_diffuse;
					Vk::ImageView m_diffuse_view;
					Vk::ImageAllocation m_diffuse_acc;
					Vk::ImageView m_diffuse_acc_view;
					Vk::ImageAllocation m_direct_light;
					Vk::ImageView m_direct_light_view;
					Vk::ImageAllocation m_direct_light_acc;
					Vk::ImageView m_direct_light_acc_view;

					void destroy(Renderer &r);
				};
			};
		};

		using Type = uint32_t;
		static inline constexpr Type Potato = 0;
		static inline constexpr Type Sspt = 1;	// Screen-space path tracing
		static inline constexpr Type Rtpt = 2;	// Ray-traced path tracing
		static inline constexpr Type Rtdp = 3;	// Ray-traced dynamic probes
		static inline constexpr Type Rtbp = 4;	// Ray-traced basis probes
		static inline constexpr Type MaxEnum = Rtbp + 1;

		struct Props {
			uint32_t descriptorCombinedImageSamplerCount;
			const char *fragShaderPath;
			uint32_t fragShaderColorAttachmentCount;
			uint32_t barrsPerFrame;
			uint32_t addBarrsPerFrame;
		};
	};

	IllumTechnique::Type m_illum_technique;
	bool needsAccStructure(void);

private:
	const IllumTechnique::Props &m_illum_technique_props;
	const IllumTechnique::Props& getIllumTechniqueProps(void);
	IllumTechnique::Type fitIllumTechnique(IllumTechnique::Type illumTechnique);

	Vk::BufferAllocation m_screen_vertex_buffer;
	Vk::BufferAllocation createScreenVertexBuffer(void);

	Vk::RenderPass m_opaque_pass;
	Vk::RenderPass createOpaquePass(void);

	Vk::RenderPass m_color_resolve_pass;
	Vk::RenderPass createColorResolvePass(void);
	Vk::DescriptorSetLayout m_color_resolve_set_layout;
	Vk::DescriptorSetLayout createColorResolveSetLayout(void);
	Pipeline m_color_resolve_pipeline;
	Pipeline createColorResolvePipeline(void);

	Vk::RenderPass m_depth_resolve_pass;
	Vk::RenderPass createDepthResolvePass(void);
	Vk::DescriptorSetLayout m_depth_resolve_set_layout;
	Vk::DescriptorSetLayout createDepthResolveSetLayout(void);
	Pipeline m_depth_resolve_pipeline;
	Pipeline createDepthResolvePipeline(void);
	Pipeline m_depth_acc_pipeline;
	Pipeline createDepthAccPipeline(void);
	Vk::RenderPass m_illumination_pass;
	Vk::RenderPass createIlluminationPass(void);
	Vk::DescriptorSetLayout m_illumination_set_layout;
	Vk::DescriptorSetLayout createIlluminationSetLayout(void);
	Pipeline m_illumination_pipeline;	// VK_NULL_HANDLE on ray tracing
	Pipeline createIlluminationPipeline(void);
	IllumTechnique::Data::RayTracing::Shared m_illum_rt;
	IllumTechnique::Data::RayTracing::Shared createIllumRayTracing(void);
	IllumTechnique::Data::Rtdp::Shared m_illum_rtdp;
	IllumTechnique::Data::Rtdp::Shared createIllumRtdp(void);
	IllumTechnique::Data::Rtbp::Shared m_illum_rtbp;
	IllumTechnique::Data::Rtbp::Shared createIllumRtbp(void);
	Vk::RenderPass m_wsi_pass;
	Vk::RenderPass createWsiPass(void);
	Vk::DescriptorSetLayout m_wsi_set_layout;
	Vk::DescriptorSetLayout createWsiSetLayout(void);
	Pipeline m_wsi_pipeline;
	Pipeline createWsiPipeline(void);

	Vk::Sampler m_sampler_fb;
	Vk::Sampler createSamplerFb(void);
	Vk::Sampler m_sampler_fb_mip;
	Vk::Sampler createSamplerFbMip(void);
	Vk::Sampler m_sampler_fb_lin;
	Vk::Sampler createSamplerFbLin(void);

	Vk::DescriptorPool m_descriptor_pool;
	Vk::DescriptorPool createDescriptorPool(void);
	Vk::DescriptorPool m_descriptor_pool_mip;
	Vk::DescriptorPool createDescriptorPoolMip(void);

	class Frame
	{
		Renderer &m_r;
		size_t m_i;
		Vk::CommandBuffer m_cmd_gtransfer;
		Vk::CommandBuffer m_cmd_grender_pass;
		Vk::CommandBuffer m_cmd_gwsi;
		Vk::CommandBuffer m_cmd_ctransfer;
		Vk::CommandBuffer m_cmd_ctrace_rays;
		Vk::Fence m_frame_done;
		Vk::Semaphore m_render_done;
		Vk::Semaphore m_image_ready;
		Vk::Semaphore m_render_pass_done;
		Vk::Semaphore m_trace_rays_done;
		bool m_ever_submitted = false;

		VkDescriptorSet m_descriptor_set_0;
		VkDescriptorSet m_descriptor_set_dynamic;

		static inline constexpr size_t dyn_buffer_size = 2000000;	// large for now, to eventually reduce to 2MB
		size_t m_dyn_buffer_size;
		void *m_dyn_buffer_staging_ptr;
		Vk::BufferAllocation m_dyn_buffer_staging;
		Vk::BufferAllocation createDynBufferStaging(void);
		Vk::BufferAllocation m_dyn_buffer;

		friend class Renderer;

		Vk::ImageView createFbImage(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation);
		Vk::ImageView createFbImageMs(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation);
		Vk::ImageView createFbImageResolved(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation);
		Vk::ImageView createFbImageMip(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation);

		Vk::ImageAllocation m_depth_buffer;
		Vk::ImageView m_depth_buffer_view;
		Vk::ImageAllocation m_cdepth;
		Vk::ImageView m_cdepth_view;
		Vk::ImageAllocation m_albedo;
		Vk::ImageView m_albedo_view;
		Vk::ImageAllocation m_normal;
		Vk::ImageView m_normal_view;
		Vk::ImageAllocation m_normal_geom;
		Vk::ImageView m_normal_geom_view;

		IllumTechnique::Data::Sspt::Fbs m_illum_ssgi_fbs;
		IllumTechnique::Data::Sspt::Fbs createIllumSsgiFbs(VkDescriptorSet descriptorSetColorResolve, VkDescriptorSet descriptorSetDepthResolve,
			const VkDescriptorSet *pDescriptorSetsMip);
		IllumTechnique::Data::RayTracing::Fbs m_illum_rt;
		IllumTechnique::Data::RayTracing::Fbs createIllumRtFbs(VkDescriptorSet descriptorSetRes);
		IllumTechnique::Data::Rtpt::Fbs m_illum_rtpt;
		IllumTechnique::Data::Rtpt::Fbs createIllumRtptFbs(void);
		IllumTechnique::Data::Rtdp::Fbs m_illum_rtdp;
		IllumTechnique::Data::Rtdp::Fbs createIllumRtdpFbs(void);
		IllumTechnique::Data::Rtbp::Fbs m_illum_rtbp;
		IllumTechnique::Data::Rtbp::Fbs createIllumRtbpFbs(void);
		Vk::ImageAllocation m_output;
		Vk::ImageView m_output_view;

		Vk::Framebuffer m_opaque_fb;
		Vk::Framebuffer createOpaqueFb(void);
		Vk::Framebuffer m_illumination_fb;
		Vk::Framebuffer createIlluminationFb(void);
		VkDescriptorSet m_illumination_set;
		Vk::BufferAllocation m_illumination_buffer;
		Vk::BufferAllocation createIlluminationBuffer(void);
		Vk::Framebuffer m_wsi_fb;
		Vk::Framebuffer createWsiFb(void);
		VkDescriptorSet m_wsi_set;

		struct Illumination {
			glm::mat4 cam_proj;
			glm::mat4 view;
			glm::mat4 view_normal;
			glm::mat4 view_inv;
			glm::mat4 view_normal_inv;
			glm::mat4 last_view;
			glm::mat4 last_view_inv;
			glm::mat4 view_cur_to_last;
			glm::mat4 view_last_to_cur;
			glm::mat4 view_last_to_cur_normal;
			glm::vec4 rnd_sun[8192];
			glm::vec4 rnd_diffuse[8192];
			glm::vec3 sun;
			float _pad;
			glm::vec2 size;
			glm::vec2 size_inv;
			glm::vec2 depth_size;
			glm::vec2 depth_size_inv;
			glm::vec2 ratio;
			glm::ivec2 probe_extent;	// rtdp
			uint32_t probe_size_l2;
			uint32_t probe_size;
			float cam_near;
			float cam_far;
			float cam_a;
			float cam_b;
		};

	public:
		Frame(Renderer &r, size_t i, Vk::CommandBuffer cmdGtransfer, Vk::CommandBuffer cmdGrenderPass, Vk::CommandBuffer cmdGwsi,
			Vk::CommandBuffer cmdCtransfer, Vk::CommandBuffer cmdCtraceRays,
			VkDescriptorSet descriptorSet0, VkDescriptorSet descriptorSetDynamic,
			VkDescriptorSet descriptorSetColorResolve, VkDescriptorSet descriptorSetDepthResolve,
			VkDescriptorSet descriptorSetIllum, VkDescriptorSet descriptorSetRayTracingRes,
			VkDescriptorSet descriptorSetWsi,
			const VkDescriptorSet *pDescriptorSetsMip,
			Vk::BufferAllocation dynBuffer);

		void reset(void);
		void render(Map &map, const Camera &camera);
		void destroy(bool with_ext_res = false);

	private:
		void render_subset(Map &map, cmp_id render_id);
	};

	vector<Frame> m_frames;
	vector<Frame> createFrames(void);
	size_t m_current_frame = 0;

	void bindFrameDescriptors(void);
	void recreateSwapchain(void);

	Vk::ShaderModule loadShaderModule(VkShaderStageFlagBits stage, const char *path) const;
	static VkPipelineShaderStageCreateInfo initPipelineStage(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

public:
	//Pipeline createPipeline(const char *stagesPath, uint32_t pushConstantRange);
	Pipeline createPipeline3D_pn(const char *stagesPath, uint32_t pushConstantRange);
	Pipeline createPipeline3D_pnu(const char *stagesPath, uint32_t pushConstantRange);
	Pipeline createPipeline3D_pntbu(const char *stagesPath, uint32_t pushConstantRange);

private:
	Pool<Pipeline> m_pipeline_pool;
public:
	Pool<Model> m_model_pool;
	Pool<AccelerationStructure> m_acc_pool;
private:
	Pool<Material> m_material_pool;
	bool image_pool_not_bound = true;
	Pool<Vk::ImageAllocation> m_image_pool;
	Pool<VkImageView> m_image_view_pool;

public:
	Pipeline *pipeline_opaque_uvgen;
	Pipeline *pipeline_opaque;
	Pipeline *pipeline_opaque_tb;

	Vk::Sampler sampler_norm_l;
	Vk::Sampler sampler_norm_n;

	uint32_t allocateImage(Vk::ImageAllocation image, bool linearSampler = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
	uint32_t allocateMaterial(void);

	Vk::BufferAllocation createVertexBuffer(size_t size);
	Vk::BufferAllocation createIndexBuffer(size_t size);
	void loadBuffer(VkBuffer buffer, size_t size, const void *data);
	void loadBufferCompute(VkBuffer buffer, size_t size, const void *data, size_t offset = 0);
	Model loadModel(const char *path, AccelerationStructure *acc);
	Model loadModelTb(const char *path, AccelerationStructure *acc);
	void instanciateModel(Map &map, const char *path, const char *filename);
	Vk::ImageAllocation loadImage(const char *path, bool gen_mips = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
	Vk::ImageAllocation loadHeightGenNormal(const char *path, bool gen_mips = true); // VK_FORMAT_R8G8B8A8_UNORM
	Vk::ImageAllocation loadImage(size_t w, size_t h, void *data, bool gen_mips = true, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);	// assumes 32bpp

	AccelerationStructure createBottomAccelerationStructure(uint32_t vertexCount, size_t vertexStride, VkBuffer vertices,
		VkIndexType indexType, uint32_t indexCount, VkBuffer indices, VkGeometryFlagsKHR flags);
	void destroy(AccelerationStructure &accelerationStructure);

	void bindCombinedImageSamplers(uint32_t firstSampler, uint32_t imageInfoCount, const VkDescriptorImageInfo *pImageInfos);

private:
	Material_albedo m_materials_albedo[materialPoolSize];

public:
	void bindMaterials_albedo(uint32_t firstMaterial, uint32_t materialCount, Material_albedo *pMaterials);
	void bindModel_pnu(uint32_t binding, VkBuffer vertexBuffer);
	void bindModel_pn_i16(uint32_t binding, VkBuffer vertexBuffer, VkBuffer indexBuffer);
	void bindModel_pntbu(uint32_t binding, VkBuffer vertexBuffer);

private:
	bool m_keys_prev[GLFW_KEY_LAST];
	bool m_keys[GLFW_KEY_LAST];
	glm::dvec2 m_cursor = glm::dvec2(0.0, 0.0);
	static inline constexpr size_t key_update_count = 9;
	static size_t m_keys_update[key_update_count];
	size_t m_pending_cursor_mode = ~0ULL;

	std::mt19937_64 m_rnd;

public:
	Renderer(uint32_t frameCount, bool validate, bool useRenderDoc);
	~Renderer(void);

	void pollEvents(void);
	bool shouldClose(void) const;
	bool keyState(int glfw_key) const;
	bool keyPressed(int glfw_key) const;
	bool keyReleased(int glfw_key) const;
	glm::dvec2 cursor(void) const;

	void setCursorMode(bool show);

	void resetFrame(void);
	void render(Map &map, const Camera &camera);

	double zrand(void);
};

}