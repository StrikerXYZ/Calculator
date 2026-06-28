// Copyright (c) 2026 Striker

#include "../core/renderer.h"

#include "../core/types.h"

#include "../core/platform.h"
#include "../core/console.h"
#include "../core/file.h"
#include "../core/debug.h"


#include "vulkan/vulkan.h"

#include "../platform/platform_windows.h"
#include "vulkan/vulkan_win32.h"

#define VULKAN_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"
#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"

#define FEATURE_VULKAN_DEBUG 1

#define MAX_UI_QUADS 1024

#define MAX_TEXT_QUADS 1024
#define PIXEL_SIZE 4	// 4 bytes
#define QUAD_INDICES 6	// 2 triangles

extern int _fltused;
int _fltused = 0;



typedef struct
{
	F4 x;
	F4 y;
} Position;

typedef struct
{
	F4 u;
	F4 v;
} UV;

typedef struct
{
	F4 r;
	F4 g;
	F4 b;
} Color;

typedef struct
{
	Position position;
	UV uv;
	Color color;
} Vertex;

typedef struct
{
	U2 atlas_x;		// top left pixel
	U2 atlas_y;
	U2 atlas_w;		// glyph size
	U2 atlas_h;
	S2 offset_x;	// draw offset
	S2 offset_y;
	U2 advance;		// how far to move cursor
	U2 _pad;
} GlyphInfo; //16b

#define GLYPH_RANGE_START 32 //ASCII space
#define GLYPH_RANGE_END 127 //ASCII del (exclusive)
#define GLYPH_COUNT (GLYPH_RANGE_END - GLYPH_RANGE_START)

typedef struct
{
	GlyphInfo glyphs[GLYPH_COUNT];
	U2 atlas_width;
	U2 atlas_height;
	U2 line_height;	// vertical step in pixels (ascender + descender + gap)
	U2 font_size;	// pixel size atlas was baked at
} FontMetrics;

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_SWAPCHAIN_IMAGES 3
typedef struct {
	VkCommandPool command_pool; //8b
	VkCommandBuffer command_buffer; //8b
	VkSemaphore semaphore_presentation_complete; //8b
	VkFence fence_draw_complete; //8b
} Frame;


typedef struct {
	VkBuffer buffer; //8b
	VkDeviceMemory memory; //8b
} VulkanBuffer;

typedef struct {
	VkImage image; //8b
	VkImageView view; //8b
	VkDeviceMemory memory; //8b
	VkSampler sampler; //8b
} VulkanImage; //32b

typedef struct {
	VkPipeline image_pipeline;
	VkPipelineLayout image_pipeline_layout;
	VkPipeline text_pipeline;
	VkPipelineLayout text_pipeline_layout;
	VkShaderModule shader_module; //8b
} VulkanPipelines;

typedef struct
{
	//VkPipeline pipeline;
	//VkPipelineLayout pipeline_layout;

	//quads
	VulkanBuffer quad_vertex_buffer; //16b
	VulkanBuffer quad_index_buffer; //16b
	Vertex* quad_vertices;	//cpu pointer to mapped memory
	U2* quad_indices;	//cpu pointer to mapped memory
	U4 quad_count;		//quads to draw this frame

	//texture

	VulkanBuffer vertex_buffer; //16b
	VulkanBuffer index_buffer; //16b
	VulkanImage texture; // 32b

	VkDescriptorSet descriptor_set; //8b

	//font

	//font atlas
	VulkanImage font; // 32b
	VkDescriptorSet font_descriptor_set;
	VkDescriptorSetLayout font_descriptor_set_layout;
	FontMetrics font_metrics;

	//text geometry
	VulkanBuffer text_vertex_buffer; //16b
	VulkanBuffer text_index_buffer; //16b
	Vertex* text_vertices;	//cpu pointer to mapped memory
	U2* text_indices;		//cpu pointer to mapped memory
	U4 text_quad_count;		//quads to draw this frame
} Render;

typedef struct
{
	VkInstance instance; //8b
	VkSurfaceKHR surface; //8b
	VkDebugUtilsMessengerEXT debug_messenger; //8b
} VulkanPlatform;

typedef struct
{
	VkPhysicalDevice physical_device; //8b
	VkDevice device; //8b
	VkQueue queue; //8b
	U4 graphics_queue_family_index; //4b
} VulkanInterface;

typedef struct
{
	VkSwapchainKHR swapchain; //8b

	VkImage* images; //8b
	VkImageView* image_views; //8b
	U4 image_count; //4b

	VkExtent2D extent;
	VkFormat format;

} VulkanPresent;

typedef struct {

	VulkanPlatform platform;
	VulkanInterface interface;
	VulkanPresent present;
	VulkanPipelines pipelines;

	//Device
	VkCommandPool transfer_command_pool;

	//Render
	//VkDevice logical_device
	Render render;

	//Present
	VkSemaphore semaphore_render_complete[MAX_SWAPCHAIN_IMAGES]; //8b

	//Frame
	Frame frames[MAX_FRAMES_IN_FLIGHT];
	U4 frame_index; //4b



	//WindowInfo window_info; //8b
	//init
	Handle window_handle; //8b
	//Texture
	//Descriptor Set
	VkDescriptorSetLayout descriptor_set_layout; //8b
	VkDescriptorPool descriptor_pool; //8b

} RendererContextVulkan; //256b

internal VKAPI_ATTR VkBool32 VKAPI_CALL renderer_callback_debug_message(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
	platform_console_log(callback_data->pMessage, buffer_find_terminator_position(callback_data->pMessage));
	platform_console_log("\n", 1);

	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		bebug_breakpoint();
	}

	return VK_FALSE;
}

internal void renderer_create_buffer(
	VulkanBuffer* out_buffer,
	VkDevice device,
	VkDeviceSize buffer_size,
	VkBufferUsageFlags buffer_usage_flags,
	const VkPhysicalDeviceMemoryProperties* physical_memory_properties,
	VkMemoryPropertyFlags memory_property_flags)
{
	DEBUG_RUNTIME_ASSERT(buffer_size, "0 size not supported");

	VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = buffer_size,
		.usage = buffer_usage_flags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VkResult result = vkCreateBuffer(device, &buffer_create_info, NULL, &out_buffer->buffer);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "buffer creation failed");

	// Find memory type index with required properties
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, out_buffer->buffer, &memory_requirements);
	U4 memory_type_index = U4_MAX;
	for (U4 i = 0; i < physical_memory_properties->memoryTypeCount; ++i)
	{
		// Check if memory requirements has required properties
		if ((memory_requirements.memoryTypeBits & (1 << i)) == 0) continue;

		// Check if memory type has required properties
		if ((physical_memory_properties->memoryTypes[i].propertyFlags & memory_property_flags) != memory_property_flags) continue;

		memory_type_index = i;
		break;
	}
	DEBUG_RUNTIME_ASSERT(memory_type_index != U4_MAX, "property flags not found");

	VkMemoryAllocateInfo memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = memory_type_index
	};

	result = vkAllocateMemory(device, &memory_allocate_info, NULL, &out_buffer->memory);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "buffer memory allocation failed");

	result = vkBindBufferMemory(device, out_buffer->buffer, out_buffer->memory, 0);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "bind buffer memory failed");
}

internal void renderer_destroy_buffer(
	VkDevice device,
	VulkanBuffer buffer_memory
)
{
	vkDestroyBuffer(device, buffer_memory.buffer, NULL);
	vkFreeMemory(device, buffer_memory.memory, NULL);
}

internal void renderer_create_buffer_stage_pair(
	VulkanBuffer* out_cpu_buffer,
	VulkanBuffer* out_gpu_buffer,
	VkDevice device,
	VkDeviceSize buffer_size,
	const VkPhysicalDeviceMemoryProperties* physical_memory_properties,
	VkBufferUsageFlags usuage_flags
)
{
	renderer_create_buffer(out_cpu_buffer,
		device, buffer_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		physical_memory_properties,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	// Create vertex data with transfer dst usage for copying from staging data
	renderer_create_buffer(out_gpu_buffer,
		device, buffer_size,
		usuage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		physical_memory_properties,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);
}

internal void renderer_stage_buffer(
	VulkanBuffer* out_cpu_buffer,
	VulkanBuffer* out_gpu_buffer,
	VkDevice device,
	void* buffer_data,
	VkDeviceSize buffer_size,
	VkCommandPool transfer_command_pool,
	VkQueue queue
)
{
	void* staging_data;
	VkResult result = vkMapMemory(device, out_cpu_buffer->memory, 0, buffer_size, 0, (void**)&staging_data);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "cpu buffer map memory failed");

	memory_copy(staging_data, buffer_data, buffer_size);
	vkUnmapMemory(device, out_cpu_buffer->memory);

	// Copy data from staging to vertex data
	VkCommandBufferAllocateInfo allocate_command_buffers_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = transfer_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkCommandBuffer copy_command_buffer;
	result = vkAllocateCommandBuffers(device, &allocate_command_buffers_info, &copy_command_buffer);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "allocate command buffer failed");
	VkCommandBufferBeginInfo command_buffer_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	result = vkBeginCommandBuffer(copy_command_buffer, &command_buffer_begin_info);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "begin command buffer failed");
	VkBufferCopy buffer_copy_region = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = buffer_size
	};
	vkCmdCopyBuffer(copy_command_buffer, out_cpu_buffer->buffer, out_gpu_buffer->buffer, 1, &buffer_copy_region);
	result = vkEndCommandBuffer(copy_command_buffer);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "end command buffer failed");
	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &copy_command_buffer
	};
	result = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "queue submit failed");
	// We have a single copy to wait for, so we can wait for the queue to be idle instead of using a fence
	// A fence would be more appropriate if we had multiple operations to wait for or if we wanted to avoid stalling the entire queue
	// TODO: use fence here
	result = vkQueueWaitIdle(queue);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "queue wait idle failed");

	vkFreeCommandBuffers(device, transfer_command_pool, 1, &copy_command_buffer);
}

internal void renderer_create_buffer_static(
	VulkanBuffer* out_buffer,
	VkDevice device,
	void* buffer_data,
	VkDeviceSize buffer_size,
	const VkPhysicalDeviceMemoryProperties* physical_memory_properties,
	VkBufferUsageFlags usuage_flags,
	VkCommandPool transfer_command_pool,
	VkQueue queue
)
{
	// Create staging data - CPU side data for copying to GPU
	VulkanBuffer staging_buffer;

	renderer_create_buffer_stage_pair(
		&staging_buffer,
		out_buffer,
		device,
		buffer_size,
		physical_memory_properties,
		usuage_flags
	);

	renderer_stage_buffer(
		&staging_buffer,
		out_buffer,
		device,
		buffer_data,
		buffer_size,
		transfer_command_pool,
		queue
	);

	renderer_destroy_buffer(device, staging_buffer);
}

internal U1 renderer_create_image(
	VulkanImage* out_image,
	VkDevice device,
	VkQueue queue,
	void* pixel_data,
	U4 texture_width, 
	U4 texture_height, 
	VkFormat texture_format,
	VkCommandPool transfer_command_pool,
	const VkPhysicalDeviceMemoryProperties* physical_memory_properties
)
{
	//4 bytes per pixel for RGBA8 format
	VkDeviceSize image_size = texture_width * texture_height * 4;

	//Staging data
	VulkanBuffer staging_buffer;
	renderer_create_buffer(
		&staging_buffer,
		device,
		image_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		physical_memory_properties, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void* mapped_data;
	vkMapMemory(device, staging_buffer.memory, 0, image_size, 0, &mapped_data);
	memory_copy(mapped_data, pixel_data, image_size);
	vkUnmapMemory(device, staging_buffer.memory);

	//Create texture image
	VkImageCreateInfo image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = texture_format,
		.extent = { texture_width, texture_height, 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	vkCreateImage(device, &image_create_info, NULL, &out_image->image);

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, out_image->image, &memory_requirements);

	VkMemoryPropertyFlags memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	U4 memory_type_index = U4_MAX;
	for (U4 i = 0; i < physical_memory_properties->memoryTypeCount; ++i)
	{
		// Check if memory requirements has required properties
		if ((memory_requirements.memoryTypeBits & (1 << i)) == 0) continue;

		// Check if memory type has required properties
		if ((physical_memory_properties->memoryTypes[i].propertyFlags & memory_property_flags) != memory_property_flags) continue;

		memory_type_index = i;
		break;
	}
	DEBUG_RUNTIME_ASSERT(memory_type_index != U4_MAX, "Failed to find suitable memory type for texture image");

	VkMemoryAllocateInfo memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = memory_type_index
	};
	vkAllocateMemory(device, &memory_allocate_info, NULL, &out_image->memory);
	vkBindImageMemory(device, out_image->image, out_image->memory, 0);

	//command data for copying from staging data to texture image
	VkCommandBufferAllocateInfo allocate_command_buffer_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = transfer_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &allocate_command_buffer_info, &command_buffer);

	VkCommandBufferBeginInfo command_buffer_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

	// Barrier 1: UNDEFINED → TRANSFER_DST_OPTIMAL
	// Prepares the image to receive a copy from the staging data
	// srcStage = TOP_OF_PIPE because there's nothing before this in the queue
	// dstStage = TRANSFER because the copy happens there
	VkImageMemoryBarrier2 image_memory_barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = out_image->image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	VkDependencyInfo dependency_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &image_memory_barrier
	};
	vkCmdPipelineBarrier2(command_buffer, &dependency_info);

	// Copy from staging data to texture image
	VkBufferImageCopy buffer_image_copy_region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { texture_width, texture_height, 1 }
	};
	vkCmdCopyBufferToImage(command_buffer, staging_buffer.buffer, out_image->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy_region);

	// Barrier 2: TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL
	// Prepares the image for shader access in the fragment shader
	// srcStage = TRANSFER because that's where the copy happens
	// dstStage = FRAGMENT_SHADER because that's where the shader reads from the image
	VkImageMemoryBarrier2 image_memory_barrier2 = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = out_image->image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	VkDependencyInfo dependency_info2 = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &image_memory_barrier2
	};
	vkCmdPipelineBarrier2(command_buffer, &dependency_info2);

	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer
	};
	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkDestroyBuffer(device, staging_buffer.buffer, NULL);
	vkFreeMemory(device, staging_buffer.memory, NULL);

	// Image View
	VkImageViewCreateInfo image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = out_image->image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = texture_format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	vkCreateImageView(device, &image_view_create_info, NULL, &out_image->view);

	// Image Sampler
	VkSamplerCreateInfo sampler_create_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.minLod = 0.0f,
		.maxLod = 0.0f
	};
	vkCreateSampler(device, &sampler_create_info, NULL, &out_image->sampler);

	return 1;
}

internal void renderer_destroy_image(
	VkDevice device,
	VulkanImage image
)
{
	vkDestroySampler(device, image.sampler, NULL);
	vkDestroyImageView(device, image.view, NULL);
	vkDestroyImage(device, image.image, NULL);
	vkFreeMemory(device, image.memory, NULL);
}

internal void renderer_load_font(
	RendererContextVulkan* vulkan_context,
	Arena* arena,
	Char* file_path_atlas,
	Char* file_path_metrics,
	const VkPhysicalDeviceMemoryProperties* physical_memory_properties
)
{
	// Load metrics binary
	FileReadHandle read_handle_metrics = file_open_for_read(file_path_metrics);
	U4 bytes_read;

	const U1 metrics_header_size = 12; //glph_count(4b) atlas_w(2b) atlas_h(2b) line_height(2b) font_size(2b)
	Char metrics_header[metrics_header_size];
	file_read(read_handle_metrics, metrics_header, metrics_header_size, &bytes_read);
	DEBUG_RUNTIME_ASSERT(bytes_read == metrics_header_size, "Font metrics header read failed");

	U4 glyph_count = *(U4*)(metrics_header + 0);
	vulkan_context->render.font_metrics.atlas_width = *(U2*)(metrics_header + 4);
	vulkan_context->render.font_metrics.atlas_height = *(U2*)(metrics_header + 6);
	vulkan_context->render.font_metrics.line_height = *(U2*)(metrics_header + 8);
	vulkan_context->render.font_metrics.font_size = *(U2*)(metrics_header + 10);

	// Each glyph is 16 bytes
	U4 glyph_bytes = glyph_count * sizeof(GlyphInfo);
	file_read(read_handle_metrics, (Char*)vulkan_context->render.font_metrics.glyphs, glyph_bytes, &bytes_read);
	DEBUG_RUNTIME_ASSERT(bytes_read == glyph_bytes, "Font metrics glyph read failed");
	file_close(read_handle_metrics);

	// Load atlas pixels
	FileReadHandle read_handle_atlas = file_open_for_read(file_path_atlas);

	const U1 atlas_header_size = 8; //glph_count(4b) atlas_w(2b) atlas_h(2b) line_height(2b) font_size(2b)
	U1 header_atlas[atlas_header_size];
	file_read(read_handle_atlas, (Char*)header_atlas, atlas_header_size, &bytes_read);
	DEBUG_RUNTIME_ASSERT(bytes_read == atlas_header_size, "Font atlas header read failed");
	U4 atlas_width = *(U4*)(header_atlas + 0);
	U4 atlas_height = *(U4*)(header_atlas + 4);

	// Load pixel data
	U4 pixel_byte_count = atlas_width * atlas_height * 4;
	Binary* pixels = memory_arena_allocate(arena, pixel_byte_count);
	file_read(read_handle_atlas, (Char*)pixels, pixel_byte_count, &bytes_read);
	DEBUG_RUNTIME_ASSERT(bytes_read == pixel_byte_count, "Font atlas read failed");
	file_close(read_handle_atlas);

	// Upload to GPU
	renderer_create_image(
		&vulkan_context->render.font,
		vulkan_context->interface.device,
		vulkan_context->interface.queue,
		pixels, atlas_width, atlas_height, VK_FORMAT_R8G8B8A8_UNORM,
		vulkan_context->transfer_command_pool,
		physical_memory_properties
	);

	// Allocate and write the font descriptor set
	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = vulkan_context->descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &vulkan_context->render.font_descriptor_set_layout
	};
	VkResult result = vkAllocateDescriptorSets(
		vulkan_context->interface.device,
		&descriptor_set_allocate_info,
		&vulkan_context->render.font_descriptor_set
	);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "descriptor allocation failed");

	VkDescriptorImageInfo descriptor_image_info = {
		.sampler = vulkan_context->render.font.sampler,
		.imageView = vulkan_context->render.font.view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	VkWriteDescriptorSet write_descriptor_set = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = vulkan_context->render.font_descriptor_set,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &descriptor_image_info
	};
	vkUpdateDescriptorSets(
		vulkan_context->interface.device,
		1, &write_descriptor_set, 0, NULL
	);
}

internal void renderer_create_text_buffers(
	RendererContextVulkan* vulkan_context,
	const VkPhysicalDeviceMemoryProperties* physical_memory_properties
)
{
	VkDeviceSize vertex_size = sizeof(Vertex) * MAX_TEXT_QUADS * PIXEL_SIZE;
	VkDeviceSize index_size = sizeof(U2) * MAX_TEXT_QUADS * QUAD_INDICES;

	renderer_create_buffer(
		&vulkan_context->render.text_vertex_buffer,
		vulkan_context->interface.device, 
		vertex_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		physical_memory_properties,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	renderer_create_buffer(
		&vulkan_context->render.text_index_buffer,
		vulkan_context->interface.device,
		index_size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		physical_memory_properties,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	vkMapMemory(
		vulkan_context->interface.device,
		vulkan_context->render.text_vertex_buffer.memory,
		0, // offset
		vertex_size,
		0, // flags
		(void **) &vulkan_context->render.text_vertices
	);

	vkMapMemory(
		vulkan_context->interface.device,
		vulkan_context->render.text_index_buffer.memory,
		0, // offset
		index_size,
		0, // flags
		(void**)&vulkan_context->render.text_indices
	);

	//Pre fill indices
	for (U4 i = 0; i < MAX_TEXT_QUADS; ++i)
	{
		U4 vertex_offset = i * PIXEL_SIZE;
		U4 index_offset = i * QUAD_INDICES;
		vulkan_context->render.text_indices[index_offset + 0] = (U2)(vertex_offset + 0);
		vulkan_context->render.text_indices[index_offset + 1] = (U2)(vertex_offset + 1);
		vulkan_context->render.text_indices[index_offset + 2] = (U2)(vertex_offset + 2);
		vulkan_context->render.text_indices[index_offset + 3] = (U2)(vertex_offset + 2);
		vulkan_context->render.text_indices[index_offset + 4] = (U2)(vertex_offset + 3);
		vulkan_context->render.text_indices[index_offset + 5] = (U2)(vertex_offset + 0);
	}
}

internal void renderer_draw_image(RendererContextVulkan* vulkan_context, VkCommandBuffer command_buffer, VkImage target_image, VkImageView target_image_view, VkExtent2D extent, VkImageLayout final_layout)
{
	VkViewport viewport = (VkViewport){
		.x = 0.0f,
		.y = 0.0f,
		.width = (F4)vulkan_context->present.extent.width,
		.height = (F4)vulkan_context->present.extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	}; //24b

	VkRect2D scissor = (VkRect2D){
		.offset = { 0, 0 },
		.extent = vulkan_context->present.extent
	}; //16b

	VkImageMemoryBarrier2 image_memory_barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR,
		.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
		.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = target_image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	VkDependencyInfo dependency_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &image_memory_barrier
	};
	vkCmdPipelineBarrier2(command_buffer, &dependency_info);


	VkClearColorValue clear_color_value = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	VkClearValue clear_color = {
		.color = clear_color_value
	};
	VkRenderingAttachmentInfoKHR rendering_attachment_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
		.imageView = target_image_view,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = clear_color
	};
	VkRenderingInfoKHR rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
		.renderArea = {
			.offset = { 0, 0 },
			.extent = extent
		},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &rendering_attachment_info
	};
	vkCmdBeginRendering(command_buffer, &rendering_info);

	if (vulkan_context->render.quad_count > 0)
	{
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_context->pipelines.image_pipeline);

		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			vulkan_context->pipelines.image_pipeline_layout, 0 /* First Set */, 1 /* Decriptor Set Count */,
			&vulkan_context->render.descriptor_set, 0, NULL);

		vkCmdBindVertexBuffers(command_buffer, 0, 1, &vulkan_context->render.quad_vertex_buffer.buffer, (VkDeviceSize[]) { 0 });
		vkCmdBindIndexBuffer(command_buffer, vulkan_context->render.quad_index_buffer.buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(command_buffer, vulkan_context->render.quad_count * 6, 1, 0, 0, 0);
	}
	vkCmdEndRendering(command_buffer);

	if (vulkan_context->render.text_quad_count > 0)
	{
		VkRenderingAttachmentInfo text_rendering_attachment_info = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = target_image_view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE
		};
		VkRenderingInfo text_rendering_info = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = (VkRect2D){
				.offset = (VkOffset2D){0, 0},
				.extent = extent
			},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &text_rendering_attachment_info
		};
		vkCmdBeginRendering(command_buffer, &text_rendering_info);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_context->pipelines.text_pipeline);
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);
		vkCmdBindDescriptorSets(
			command_buffer, 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			vulkan_context->pipelines.text_pipeline_layout,
			0, 1,
			&vulkan_context->render.font_descriptor_set,
			0, NULL
		);
		vkCmdBindVertexBuffers(
			command_buffer, 0, 1,
			&vulkan_context->render.text_vertex_buffer.buffer,
			(VkDeviceSize[]){0}
		);
		vkCmdBindIndexBuffer(
			command_buffer,
			vulkan_context->render.text_index_buffer.buffer,
			0,
			VK_INDEX_TYPE_UINT16
		);
		vkCmdDrawIndexed(
			command_buffer,
			vulkan_context->render.text_quad_count * 6,
			1, 0, 0, 0
		);
		vkCmdEndRendering(command_buffer);
	}


	VkImageMemoryBarrier2 image_memory_barrier_2 = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR,
		.dstAccessMask = 0,
		.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
		.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = final_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = target_image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	VkDependencyInfo dependency_info_2 = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &image_memory_barrier_2
	};
	vkCmdPipelineBarrier2(command_buffer, &dependency_info_2);
}

internal void renderer_build_text(RendererContextVulkan* vulkan_context, StringView text, F4 x, F4 y, F4 font_size, F4 r, F4 g, F4 b)
{
	FontMetrics* font_metrics = &vulkan_context->render.font_metrics;
	F4 atlas_width = (F4)font_metrics->atlas_width;
	F4 atlas_height = (F4)font_metrics->atlas_height;
	F4 screen_width = (F4)vulkan_context->present.extent.width;
	F4 screen_height = (F4)vulkan_context->present.extent.height;

	F4 cursor_x = x;
	F4 cursor_y = y;

	for (U8 i = 0; i < text.size; ++i)
	{
		Char character = text.data[i];

		// Handle new line
		if (character == '\n')
		{
			cursor_x = x;
			cursor_y += (F4)font_metrics->line_height;
			continue;
		}

		// Handle unsupported charcters
		if (character < GLYPH_RANGE_START || character > GLYPH_RANGE_END)
			continue;

		if (vulkan_context->render.text_quad_count >= MAX_TEXT_QUADS)
			break;

		GlyphInfo* glyph_info = &font_metrics->glyphs[character - GLYPH_RANGE_START];

		// Pixel coordinate of this glyph 's quad on the screen
		F4 px0 = cursor_x + (F4)glyph_info->offset_x;
		F4 py0 = cursor_y + (F4)glyph_info->offset_y;
		F4 px1 = px0 + (F4)glyph_info->atlas_w;
		F4 py1 = py0 + (F4)glyph_info->atlas_h;

		// Convert pixel coordinate to NDC [-1, 1]
		// NDC x: (pixel / screen_width) * 2 - 1
		// NDC y: (pixel / screen_height) * 2 - 1
		F4 ndc_x0 = (px0 / screen_width) * 2.f - 1.f;
		F4 ndc_y0 = (py0 / screen_height) * 2.f - 1.f;
		F4 ndc_x1 = (px1 / screen_width) * 2.f - 1.f;
		F4 ndc_y1 = (py1 / screen_height) * 2.f - 1.f;

		// UV coordinate into atlas
		F4 u0 = (F4)glyph_info->atlas_x / atlas_width;
		F4 v0 = (F4)glyph_info->atlas_y / atlas_height;
		F4 u1 = (F4)(glyph_info->atlas_x + glyph_info->atlas_w) / atlas_width;
		F4 v1 = (F4)(glyph_info->atlas_y + glyph_info->atlas_h) / atlas_height;

		// Write 4 vertices for this quad
		U4 base = vulkan_context->render.text_quad_count * 4;
		Vertex* vertex = vulkan_context->render.text_vertices;

		//top-left
		vertex[base + 0] = (Vertex){
			.position = (Position){ndc_x0, ndc_y0},
			.uv = (UV){u0, v0},
			.color = (Color){r, g, b}
		};
		//top-right
		vertex[base + 1] = (Vertex){
			.position = (Position){ndc_x1, ndc_y0},
			.uv = (UV){u1, v0},
			.color = (Color){r, g, b}
		};
		//bottom-right
		vertex[base + 2] = (Vertex){
			.position = (Position){ndc_x1, ndc_y1},
			.uv = (UV){u1, v1},
			.color = (Color){r, g, b}
		};
		//bottom-left
		vertex[base + 3] = (Vertex){
			.position = (Position){ndc_x0, ndc_y1},
			.uv = (UV){u0, v1},
			.color = (Color){r, g, b}
		};

		cursor_x += (F4)glyph_info->advance;
		vulkan_context->render.text_quad_count++;
	}
}

internal void renderer_intialize_platform(
	VulkanPlatform* out_platform,
	Handle window_handle,
	Handle application_handle
)
{
	VkResult result; //4b

	//Vulkan layer & extension specification
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = APPLICATION_NAME,
		.applicationVersion = 0,
		.pEngineName = APPLICATION_NAME,
		.engineVersion = 0,
		.apiVersion = VK_API_VERSION_1_4
	}; //48b

	const Char* vulkan_required_layers[] = {
		VULKAN_VALIDATION_LAYER_NAME
	}; //8b

	const Char* vulkan_required_extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME //TODO: Disable in shipping
	}; //24b

	VkValidationFeatureEnableEXT enabled_features[] = {
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,   //TODO: Disable in performance
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,   //TODO: Disable in performance
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
		VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,  //TODO: Disable in performance
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
	}; //4b

	VkValidationFeaturesEXT validation_features = {
		.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
		.pEnabledValidationFeatures = enabled_features,
		.enabledValidationFeatureCount = ARRAY_COUNT(enabled_features)
	}; //48b

	VkInstanceCreateInfo instance_create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &application_info,
		.ppEnabledLayerNames = vulkan_required_layers,
#ifdef FEATURE_VULKAN_DEBUG
		.enabledLayerCount = ARRAY_COUNT(vulkan_required_layers),
#else
		.enabledLayerCount = 0,
#endif
		.ppEnabledExtensionNames = vulkan_required_extensions,
		.enabledExtensionCount = ARRAY_COUNT(vulkan_required_extensions),
		.pNext = &validation_features
	}; //64b

	// Vulkan instance that contains the API state
	result = vkCreateInstance(&instance_create_info, NULL, &out_platform->instance);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "Failed to create instance");

	//Debug messenger to catch issues
	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = renderer_callback_debug_message,
		.pUserData = NULL
	}; //48b
	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(out_platform->instance, "vkCreateDebugUtilsMessengerEXT"); //8b
	vkCreateDebugUtilsMessengerEXT(out_platform->instance, &debug_messenger_create_info, NULL, &out_platform->debug_messenger);


	// Vulkan surface for OS window binding
	VkWin32SurfaceCreateInfoKHR surface_create_info = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hwnd = window_handle,
		.hinstance = application_handle
	}; // 40b
	vkCreateWin32SurfaceKHR(out_platform->instance, &surface_create_info, NULL, &out_platform->surface);


}

internal void renderer_initialize_interface(
	VulkanInterface* out_interface,
	VulkanPlatform platform
)
{
	VkResult result; //4b

	// Vulkan physical device - GPU
	U4 physical_device_count;
	result = vkEnumeratePhysicalDevices(platform.instance, &physical_device_count, NULL);
	DEBUG_RUNTIME_ASSERT(physical_device_count > 0, "No physical devices found");

	VkPhysicalDevice physical_devices[physical_device_count];
	result = vkEnumeratePhysicalDevices(platform.instance, &physical_device_count, physical_devices);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "Failed to enumerate devices");

	//Check for discrete GPU and Vulkan 1.4 support
	VkPhysicalDeviceProperties properties; //824b
	U4 best_device_index = U4_MAX;
	for (U4 i = 0; i < physical_device_count; ++i)
	{
		vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && properties.apiVersion >= VK_API_VERSION_1_4)
		{
			best_device_index = i;
			break;
		}
	}
	DEBUG_RUNTIME_ASSERT(best_device_index != U4_MAX, "Failed to find dicrete GPU");
	out_interface->physical_device = physical_devices[best_device_index];


	// Check for extension support
	U4 device_extension_count;
	vkEnumerateDeviceExtensionProperties(out_interface->physical_device, NULL, &device_extension_count, NULL);
	DEBUG_RUNTIME_ASSERT(device_extension_count > 0, "no extension properties found");

	VkExtensionProperties device_extensions[device_extension_count]; //(260 * 212)b
	vkEnumerateDeviceExtensionProperties(out_interface->physical_device, NULL, &device_extension_count, device_extensions);
	U1 supports_extension = 0;
	for (U4 i = 0; i < device_extension_count; ++i)
	{
		const Char* extension_name = device_extensions[i].extensionName;
		if (buffer_equals(
			(Binary const*)extension_name, 
			buffer_find_terminator_position(extension_name), 
			(Binary const*)VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			buffer_find_terminator_position(VK_KHR_SWAPCHAIN_EXTENSION_NAME)))
		{
			supports_extension = 1;
			break;
		}
	}
	DEBUG_RUNTIME_ASSERT(supports_extension > 0, "no extension properties found");

	const Char* device_required_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	U4 device_extention_count = sizeof(device_required_extensions) / sizeof(device_required_extensions[0]);


	//Check for feature support
	VkPhysicalDeviceVulkan13Features feature_1_3 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = NULL
	}; //80b
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT feature_extended_dynamic_state = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
		.pNext = NULL
	}; //24b
	feature_1_3.pNext = &feature_extended_dynamic_state;
	VkPhysicalDeviceFeatures2 device_features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &feature_1_3
	}; //240b
	vkGetPhysicalDeviceFeatures2(out_interface->physical_device, &device_features);
	DEBUG_RUNTIME_ASSERT(feature_1_3.dynamicRendering && feature_extended_dynamic_state.extendedDynamicState, "Features not available");




	// Check for graphics support
	U4 queue_family_property_count;
	vkGetPhysicalDeviceQueueFamilyProperties(out_interface->physical_device, &queue_family_property_count, NULL);
	DEBUG_RUNTIME_ASSERT(queue_family_property_count > 0, "Failed to find device queue family properties");

	VkQueueFamilyProperties queue_family_properties[queue_family_property_count]; //(24 * 5)b
	vkGetPhysicalDeviceQueueFamilyProperties(out_interface->physical_device, &queue_family_property_count, queue_family_properties);
	out_interface->graphics_queue_family_index = U4_MAX;
	VkBool32 has_presentation_support = 0;
	for (U4 i = 0; i < queue_family_property_count; ++i)
	{
		if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			out_interface->graphics_queue_family_index = i;

			vkGetPhysicalDeviceSurfaceSupportKHR(out_interface->physical_device, i, platform.surface, &has_presentation_support);

			break;
		}
	}
	DEBUG_RUNTIME_ASSERT(out_interface->graphics_queue_family_index != U4_MAX && has_presentation_support != VK_FALSE, "Failed to find device queue family properties");

	//Vulkan logical device - for GPU communication
	float queue_priorities = 1.0f;
	VkDeviceQueueCreateInfo device_queue_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = out_interface->graphics_queue_family_index,
		.queueCount = 1,
		.pQueuePriorities = &queue_priorities
	};
	VkDeviceCreateInfo device_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &device_queue_create_info,
		.enabledExtensionCount = device_extention_count,
		.ppEnabledExtensionNames = device_required_extensions,
		.pNext = &device_features
	};
	result = vkCreateDevice(out_interface->physical_device, &device_create_info, NULL, &out_interface->device);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "Failed to create logical device");


	// Graphics queue
	vkGetDeviceQueue(out_interface->device, out_interface->graphics_queue_family_index, 0, &out_interface->queue);
}

internal void renderer_initialize_present(
	VulkanPresent* out_present,
	VkSurfaceKHR surface,
	VkPhysicalDevice physical_device,
	VkDevice device,
	Arena* arena
)
{
	// Select best fromat
	U4 surface_format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, NULL);
	DEBUG_RUNTIME_ASSERT(surface_format_count > 0, "Didnt find any surface formats");

	VkSurfaceFormatKHR surface_formats[surface_format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats);
	U4 best_surface_format_index = 0;
	for (U4 i = 0; i < surface_format_count; ++i)
	{
		if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			best_surface_format_index = i;
			break;
		}
	}
	VkSurfaceFormatKHR surface_format = surface_formats[best_surface_format_index];


	// Select best present mode
	U4 surface_present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &surface_present_mode_count, NULL);
	DEBUG_RUNTIME_ASSERT(surface_present_mode_count > 0, "Didnt find any present modes");

	VkPresentModeKHR surface_present_modes[surface_present_mode_count]; //(4 * 5)b
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &surface_present_mode_count, surface_present_modes);
	U4 best_surface_present_mode_index = U4_MAX;
	for (U4 i = 0; i < surface_present_mode_count; ++i)
	{
		if (surface_present_modes[i] == VK_PRESENT_MODE_FIFO_KHR)
		{
			best_surface_present_mode_index = i;
		}
		else if (surface_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			best_surface_present_mode_index = i;
			break;
		}
	}
	DEBUG_RUNTIME_ASSERT(best_surface_present_mode_index != U4_MAX, "present mode not set");
	VkPresentModeKHR present_mode = surface_present_modes[best_surface_present_mode_index];


	// Swapchain capabilities
	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

	// image count
	U4 image_count = surface_capabilities.minImageCount + 1;
	image_count = image_count > surface_capabilities.maxImageCount ? surface_capabilities.maxImageCount : image_count;


	//Create Swapchain - for presenting queued images
	VkSwapchainCreateInfoKHR swap_chain_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = image_count,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = surface_capabilities.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = surface_capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};
	VkResult result = vkCreateSwapchainKHR(device, &swap_chain_info, NULL, &out_present->swapchain);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "Could not create swapchain");

	// Swap Chain Images - Actual buffers
	vkGetSwapchainImagesKHR(device, out_present->swapchain, &image_count, NULL);
	if (out_present->image_count < image_count)
	{
		DEBUG_RUNTIME_ASSERT(out_present->image_count == 0, "Non zero case not handled");
		out_present->images = (VkImage*)memory_arena_allocate(arena, sizeof(VkImage) * image_count);
	}
	vkGetSwapchainImagesKHR(device, out_present->swapchain, &image_count, out_present->images);

	// Swap Chain Image Views - View into buffers for shaders
	if (out_present->image_count < image_count)
	{
		DEBUG_RUNTIME_ASSERT(out_present->image_count == 0, "Non zero case not handled");
		out_present->image_views = (VkImageView*)memory_arena_allocate(arena, sizeof(VkImageView) * image_count);
	}
	for (U4 i = 0; i < image_count; ++i)
	{
		VkImageViewCreateInfo image_view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = out_present->images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = surface_format.format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		result = vkCreateImageView(device, &image_view_create_info, NULL, &out_present->image_views[i]);
		DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "Could not create image view");
	}


	out_present->image_count = image_count;
	out_present->extent = surface_capabilities.currentExtent;
	out_present->format = surface_format.format;
}

internal void renderer_cleanup_present(
	VulkanPresent* present,
	VkDevice device
)
{
	for (U4 i = 0; i < present->image_count; ++i)
	{
		vkDestroyImageView(device, present->image_views[i], NULL);
	}
	vkDestroySwapchainKHR(device, present->swapchain, NULL);
}

internal void renderer_cleanup_interface(const VulkanInterface* interface)
{
	vkDestroyDevice(interface->device, NULL);
}

internal void renderer_cleanup_platform(const VulkanPlatform* platform)
{
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(platform->instance, "vkDestroyDebugUtilsMessengerEXT");
	vkDestroyDebugUtilsMessengerEXT(platform->instance, platform->debug_messenger, NULL);
	vkDestroySurfaceKHR(platform->instance, platform->surface, NULL);
	vkDestroyInstance(platform->instance, NULL);
}

internal void renderer_create_pipelines(
	VulkanPipelines* out_pipelines,
	VkDevice device,
	VkExtent2D surface_extent,
	VkFormat surface_format
)
{
	// Common info
	VkVertexInputBindingDescription vertex_input_binding_description = {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	}; //12b
	VkVertexInputAttributeDescription vertex_input_attribute_descriptions[] = {
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, position)
		},
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, uv)
		},
		{
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, color)
		}
	}; //24b
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pVertexBindingDescriptions = &vertex_input_binding_description,
		.vertexBindingDescriptionCount = 1,
		.pVertexAttributeDescriptions = vertex_input_attribute_descriptions,
		.vertexAttributeDescriptionCount = ARRAY_COUNT(vertex_input_attribute_descriptions)
	}; //48b
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	}; //32b
	VkViewport viewport = (VkViewport){
		.x = 0.0f,
		.y = 0.0f,
		.width = (F4)surface_extent.width,
		.height = (F4)surface_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	}; //24b
	VkRect2D scissor = (VkRect2D){
		.offset = { 0, 0 },
		.extent = surface_extent
	}; //16b
	VkPipelineViewportStateCreateInfo viewport_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	}; //48b
	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE, //VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f
	}; //64b
	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE
	}; //48b


	//Create pipeline stages
	const U4 buffer_size = 4096;
	Char buffer_shader[buffer_size];
	U4 bytes_read = 0;
	FileReadHandle read_handle = file_open_for_read("resources/slang.spv");
	file_read(read_handle, buffer_shader, buffer_size, &bytes_read);
	DEBUG_RUNTIME_ASSERT(buffer_size > bytes_read, "Exceeded buffer size");
	file_close(read_handle);
	VkShaderModuleCreateInfo shader_module_create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = bytes_read,
		.pCode = (const U4*)buffer_shader
	}; //40b
	vkCreateShaderModule(device, &shader_module_create_info, NULL, &out_pipelines->shader_module);
	VkPipelineShaderStageCreateInfo shader_stage_vertex_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = out_pipelines->shader_module,
		.pName = "pass_vertex"
	}; //48b


	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	}; //4b
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = ARRAY_COUNT(dynamic_states),
		.pDynamicStates = dynamic_states
	}; //32b

	VkPipelineRenderingCreateInfoKHR render_pass_rendering_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &surface_format
	}; //40b

	// Image pipeline
	VkPipelineShaderStageCreateInfo shader_stage_texture_fragment_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = out_pipelines->shader_module,
		.pName = "pass_fragment_texture"
	};
	VkPipelineShaderStageCreateInfo shader_stages_texture[] = {
		shader_stage_vertex_create_info,
		shader_stage_texture_fragment_create_info
	};
	VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
		.blendEnable = VK_FALSE,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	}; //32b
	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment_state
	}; //56b
	VkGraphicsPipelineCreateInfo image_pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = sizeof(shader_stages_texture) / sizeof(shader_stages_texture[0]),
		.pStages = shader_stages_texture,
		.pVertexInputState = &vertex_input_state_create_info,
		.pInputAssemblyState = &input_assembly_state_create_info,
		.pViewportState = &viewport_state_create_info,
		.pRasterizationState = &rasterization_state_create_info,
		.pMultisampleState = &multisample_state_create_info,
		.pColorBlendState = &color_blend_state_create_info,
		.pDynamicState = &dynamic_state_create_info,
		.layout = out_pipelines->image_pipeline_layout,
		.renderPass = VK_NULL_HANDLE,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
		.pNext = &render_pass_rendering_create_info
	}; //144b
	vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&image_pipeline_create_info,
		NULL,
		&out_pipelines->image_pipeline
	);



	// Font pipeline
	VkPipelineShaderStageCreateInfo shader_stage_font_fragment_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = out_pipelines->shader_module,
		.pName = "pass_fragment_font"
	};
	VkPipelineShaderStageCreateInfo shader_stages_font[] = {
		shader_stage_vertex_create_info,
		shader_stage_font_fragment_create_info
	};
	VkPipelineColorBlendAttachmentState text_blend = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
	VkPipelineColorBlendStateCreateInfo text_blend_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &text_blend
	};
	VkGraphicsPipelineCreateInfo text_pipeline_create_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = sizeof(shader_stages_font) / sizeof(shader_stages_font[0]),
		.pStages = shader_stages_font,
		.pVertexInputState = &vertex_input_state_create_info,
		.pInputAssemblyState = &input_assembly_state_create_info,
		.pViewportState = &viewport_state_create_info,
		.pRasterizationState = &rasterization_state_create_info,
		.pMultisampleState = &multisample_state_create_info,
		.pColorBlendState = &text_blend_state,
		.pDynamicState = &dynamic_state_create_info,
		.layout = out_pipelines->text_pipeline_layout,
		.renderPass = VK_NULL_HANDLE,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
		.pNext = &render_pass_rendering_create_info
	};
	vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&text_pipeline_create_info,
		NULL,
		&out_pipelines->text_pipeline
	);
}

internal void renderer_create_quad_buffers(
	Render* out_render,
	VkDevice device,
	const VkPhysicalDeviceMemoryProperties* physical_memory_properties,
	VkCommandPool transfer_command_pool
)
{
	VkDeviceSize vertex_size = sizeof(Vertex) * MAX_UI_QUADS * 4;
	VkDeviceSize index_size = sizeof(U2) * MAX_UI_QUADS * 6;

	renderer_create_buffer(
		&out_render->quad_vertex_buffer,
		device,
		vertex_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		physical_memory_properties,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);
	renderer_create_buffer(
		&out_render->quad_index_buffer,
		device,
		index_size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		physical_memory_properties,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	vkMapMemory(device,
		out_render->quad_vertex_buffer.memory,
		0, vertex_size, 0,
		(void**)&out_render->quad_vertices);

	vkMapMemory(device,
		out_render->quad_index_buffer.memory,
		0, index_size, 0,
		(void**)&out_render->quad_indices);

	for(U4 i = 0; i < MAX_UI_QUADS; ++i)
	{
		U4 vertex_base = i * 4;
		U4 index_base = i * 6;
		out_render->quad_indices[index_base + 0] = (U2)(vertex_base + 0);
		out_render->quad_indices[index_base + 1] = (U2)(vertex_base + 1);
		out_render->quad_indices[index_base + 2] = (U2)(vertex_base + 2);
		out_render->quad_indices[index_base + 3] = (U2)(vertex_base + 2);
		out_render->quad_indices[index_base + 4] = (U2)(vertex_base + 3);
		out_render->quad_indices[index_base + 5] = (U2)(vertex_base + 0);
	}

}

U1 renderer_initialize(
	RendererContext* renderer_context, 
	Arena* arena, 
	Handle window_handle, 
	Handle application_handle
)
{
	VkResult result; //4b

	// allocate vulkan context on heap
	*renderer_context = (RendererContext*)memory_arena_allocate(arena, sizeof(RendererContextVulkan));
	RendererContextVulkan* vulkan_context = (RendererContextVulkan*)(*renderer_context);

	vulkan_context->window_handle = window_handle;

	renderer_intialize_platform(
		&vulkan_context->platform,
		window_handle,
		application_handle
	);

	renderer_initialize_interface(
		&vulkan_context->interface,
		vulkan_context->platform
	);

	renderer_initialize_present(
		&vulkan_context->present,
		vulkan_context->platform.surface,
		vulkan_context->interface.physical_device,
		vulkan_context->interface.device,
		arena
	);

	VkPhysicalDeviceMemoryProperties physical_memory_properties;
	vkGetPhysicalDeviceMemoryProperties(vulkan_context->interface.physical_device, &physical_memory_properties);


	VkDescriptorSetLayoutBinding descriptor_binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = NULL
	}; //32b
	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &descriptor_binding
	}; //48b
	vkCreateDescriptorSetLayout(vulkan_context->interface.device, &descriptor_set_layout_create_info, NULL, &vulkan_context->descriptor_set_layout);

	//Create pipeline layout - Descriptor + Push Constant
	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &vulkan_context->descriptor_set_layout,
		.pushConstantRangeCount = 0
	}; //48b
	vkCreatePipelineLayout(vulkan_context->interface.device, &pipeline_layout_create_info, NULL, &vulkan_context->pipelines.image_pipeline_layout);


	//Create image pipeline 


	// Transfer command pool
	VkCommandPoolCreateInfo command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = vulkan_context->interface.graphics_queue_family_index
	}; //24b
	vkCreateCommandPool(vulkan_context->interface.device, &command_pool_create_info, NULL, &vulkan_context->transfer_command_pool);

	// Staged buffers
#if 0
	renderer_create_staged_buffer(
		&vulkan_context->render.vertex_buffer,
		&vulkan_context->interface,
		vertices, sizeof(vertices),
		&physical_memory_properties,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vulkan_context->transfer_command_pool
	);
	renderer_create_staged_buffer(
		&vulkan_context->render.index_buffer,
		&vulkan_context->interface,
		indices, sizeof(indices),
		&physical_memory_properties,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		vulkan_context->transfer_command_pool
	);
#endif

	// Semaphores - GPU:GPU sync
	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	}; //24b

	//Fences - GPU:CPU sync
	VkFenceCreateInfo fence_create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	}; //24b

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		//Command pool - for allocating command buffers
		VkCommandPoolCreateInfo command_pool_create_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = vulkan_context->interface.graphics_queue_family_index
		}; //24b
		vkCreateCommandPool(vulkan_context->interface.device, &command_pool_create_info, NULL, &vulkan_context->frames[i].command_pool);

		//Command data - Recording GPU work
		VkCommandBufferAllocateInfo command_buffer_allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = vulkan_context->frames[i].command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		}; //32b
		vkAllocateCommandBuffers(vulkan_context->interface.device, &command_buffer_allocate_info, &vulkan_context->frames[i].command_buffer);

		vkCreateSemaphore(vulkan_context->interface.device, &semaphore_create_info, NULL, &vulkan_context->frames[i].semaphore_presentation_complete);
		vkCreateFence(vulkan_context->interface.device, &fence_create_info, NULL, &vulkan_context->frames[i].fence_draw_complete);
		
	}

	for (int i = 0; i < MAX_SWAPCHAIN_IMAGES; ++i)
	{
		vkCreateSemaphore(vulkan_context->interface.device, &semaphore_create_info, NULL, &vulkan_context->semaphore_render_complete[i]);
	}

	// Test checkerboard texture
#if 0
	U1 checkerboard_pixels[4 * 4 * 4]; // 4x4 pixels, 4 bytes each
	for (U4 y = 0; y < 4; ++y)
	{
		for (U4 x = 0; x < 4; ++x)
		{
			U4 index = (y * 4 + x) * 4;
			U1 is_white = (x + y) % 2 == 0;
			checkerboard_pixels[index + 0] = is_white ? 0xFF : 0x40; // R
			checkerboard_pixels[index + 1] = is_white ? 0xFF : 0x40; // G
			checkerboard_pixels[index + 2] = is_white ? 0xFF : 0x40; // B
			checkerboard_pixels[index + 3] = 0xFF; // A
		}
	}
#endif
	U4 white_pixel = 0xFFFFFFFF;
	renderer_create_image(
		&vulkan_context->render.texture,
		vulkan_context->interface.device,
		vulkan_context->interface.queue,
		&white_pixel,
		1, 1, VK_FORMAT_R8G8B8A8_SRGB,
		vulkan_context->transfer_command_pool,
		&physical_memory_properties
	);

	//Quads
	renderer_create_quad_buffers(
		&vulkan_context->render,
		vulkan_context->interface.device,
		&physical_memory_properties,
		vulkan_context->transfer_command_pool
	);


	// Descriptor Pool
	VkDescriptorPoolSize pool_size = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 2
	};
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 2,
		.poolSizeCount = 1,
		.pPoolSizes = &pool_size
	};
	vkCreateDescriptorPool(vulkan_context->interface.device, &descriptor_pool_create_info, NULL, &vulkan_context->descriptor_pool);

	//Allocate One Descriptor
	VkDescriptorSetAllocateInfo descriptor_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = vulkan_context->descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &vulkan_context->descriptor_set_layout
	};
	result = vkAllocateDescriptorSets(vulkan_context->interface.device, &descriptor_allocate_info, &vulkan_context->render.descriptor_set);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "descriptor allocation failed");

	//Write into descriptor set
	VkDescriptorImageInfo descriptor_image_info = {
		.sampler = vulkan_context->render.texture.sampler,
		.imageView = vulkan_context->render.texture.view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	VkWriteDescriptorSet descriptor_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = vulkan_context->render.descriptor_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &descriptor_image_info
	};
	vkUpdateDescriptorSets(vulkan_context->interface.device, 1, &descriptor_write, 0, NULL);

	// Text
	renderer_create_text_buffers(vulkan_context, &physical_memory_properties);

	// Text pipeline
	VkDescriptorSetLayoutBinding font_binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	VkDescriptorSetLayoutCreateInfo font_layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &font_binding
	};
	vkCreateDescriptorSetLayout(
		vulkan_context->interface.device, 
		&font_layout_info, 
		NULL, 
		&vulkan_context->render.font_descriptor_set_layout
	);
	renderer_load_font(
		vulkan_context,
		arena,
		"resources/font.rawatlas",
		"resources/font.rawmetrics",
		&physical_memory_properties
	);

	VkPipelineLayoutCreateInfo text_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &vulkan_context->render.font_descriptor_set_layout
	};
	vkCreatePipelineLayout(
		vulkan_context->interface.device,
		&text_layout_info,
		NULL,
		&vulkan_context->pipelines.text_pipeline_layout
	);

	// create font pipeline

	renderer_create_pipelines(
		&vulkan_context->pipelines,
		vulkan_context->interface.device,
		vulkan_context->present.extent,
		vulkan_context->present.format
	);

	// Index for frame rotation - Double Buffering
	vulkan_context->frame_index = 0;

	return 1;
}

void renderer_begin_frame(RendererContext renderer_context)
{
	// Reset all per-frame geometry counts before the caller pushes new data
	RendererContextVulkan* vulkan_context = (RendererContextVulkan*)renderer_context;
	vulkan_context->render.quad_count = 0;
	vulkan_context->render.text_quad_count = 0;
}

void renderer_push_quad(RendererContext renderer_context, F4 x, F4 y, F4 w, F4 h, F4 r, F4 g, F4 b)
{
	RendererContextVulkan* vulkan_context = (RendererContextVulkan*)renderer_context;

	DEBUG_RUNTIME_ASSERT(vulkan_context->render.quad_count < MAX_UI_QUADS, "Exceeded max quad count");

	F4 surface_width = (F4)vulkan_context->present.extent.width;
	F4 surface_height = (F4)vulkan_context->present.extent.height;

	// Convert to NDC
	F4 ndc_left = (x / surface_width) * 2.f - 1.f;
	F4 ndc_right = ((x + w) / surface_width) * 2.f - 1.f;
	F4 ndc_top = (y / surface_height) * 2.f - 1.f;
	F4 ndc_bottom = ((y + h) / surface_height) * 2.f - 1.f;

	U4 base_index = vulkan_context->render.quad_count * 4;
	Vertex* vertices = vulkan_context->render.quad_vertices;

	vertices[base_index + 0] = (Vertex){ 
		.position = { ndc_left, ndc_top }, 
		.uv = { 0.f, 0.f }, 
		.color = { r, g, b } 
	};
	vertices[base_index + 1] = (Vertex){
		.position = { ndc_right, ndc_top },
		.uv = { 1.f, 0.f },
		.color = { r, g, b }
	};
	vertices[base_index + 2] = (Vertex){
		.position = { ndc_right, ndc_bottom },
		.uv = { 1.f, 1.f },
		.color = { r, g, b }
	};
	vertices[base_index + 3] = (Vertex){
		.position = { ndc_left, ndc_bottom },
		.uv = { 0.f, 1.f },
		.color = { r, g, b }
	};

	vulkan_context->render.quad_count++;
}

void renderer_push_text(RendererContext renderer_context, StringView string_view, F4 x, F4 y, F4 font_size, F4 r, F4 g, F4 b)
{
	renderer_build_text(renderer_context, string_view, x, y, font_size, r, g, b);
}

void renderer_draw_frame(RendererContext renderer_context, Arena* arena, U1 window_resized)
{
	RendererContextVulkan* vulkan_context = (RendererContextVulkan*)renderer_context;

	Frame* frame = &vulkan_context->frames[vulkan_context->frame_index];

	// Wait for frame
	VkResult result = vkWaitForFences(vulkan_context->interface.device, 1, &frame->fence_draw_complete, VK_TRUE, U8_MAX);
	DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "Wait for Fence failed");

	// Acquire image
	U4 image_index;
	result = vkAcquireNextImageKHR(vulkan_context->interface.device, vulkan_context->present.swapchain, U8_MAX, frame->semaphore_presentation_complete, VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		//window_resized - may change earlier but the semaphores may be locked

		vkDeviceWaitIdle(vulkan_context->interface.device);
		renderer_cleanup_present(&vulkan_context->present, vulkan_context->interface.device);
		renderer_initialize_present(
			&vulkan_context->present,
			vulkan_context->platform.surface,
			vulkan_context->interface.physical_device,
			vulkan_context->interface.device,
			arena
		);

		result = vkAcquireNextImageKHR(
			vulkan_context->interface.device,
			vulkan_context->present.swapchain,
			U8_MAX, 
			frame->semaphore_presentation_complete,
			VK_NULL_HANDLE,
			&image_index);

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			// Window still changing faster than we can keep up — skip this frame
			return;
		}
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		bebug_breakpoint();
		return;
	}

	vkResetFences(vulkan_context->interface.device, 1, &frame->fence_draw_complete);
	vkResetCommandPool(vulkan_context->interface.device, frame->command_pool, 0);

	VkCommandBufferBeginInfo command_buffer_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};
	vkBeginCommandBuffer(frame->command_buffer, &command_buffer_begin_info);

	renderer_draw_image(vulkan_context, frame->command_buffer, vulkan_context->present.images[image_index], vulkan_context->present.image_views[image_index], vulkan_context->present.extent, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	vkEndCommandBuffer(frame->command_buffer);

	VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pWaitSemaphores = &frame->semaphore_presentation_complete,
		.waitSemaphoreCount = 1,
		.pWaitDstStageMask = &wait_stage_mask,
		.pCommandBuffers = &frame->command_buffer,
		.commandBufferCount = 1,
		.pSignalSemaphores = &vulkan_context->semaphore_render_complete[image_index],
		.signalSemaphoreCount = 1
	};
	vkQueueSubmit(vulkan_context->interface.queue, 1, &submit_info, frame->fence_draw_complete);

	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vulkan_context->semaphore_render_complete[image_index],
		.swapchainCount = 1,
		.pSwapchains = &vulkan_context->present.swapchain,
		.pImageIndices = &image_index,
		.pResults = NULL
	};
	result = vkQueuePresentKHR(vulkan_context->interface.queue, &present_info);

	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) // || Frame data resized
	{
		//todo: recreate swapchain
		//bebug_breakpoint();

#if 0	// Currently breaks on close
		vkDeviceWaitIdle(vulkan_context->interface.device);
		renderer_cleanup_present(&vulkan_context->present, vulkan_context->interface.device);
		renderer_initialize_present(
			&vulkan_context->present,
			vulkan_context->platform.surface,
			vulkan_context->interface.physical_device,
			vulkan_context->interface.device,
			arena
		);
#endif

		return;
	}
	else
	{
		DEBUG_RUNTIME_ASSERT(result == VK_SUCCESS, "Failed to present graphics queue");
	}
	vulkan_context->frame_index = (vulkan_context->frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

void renderer_cleanup(RendererContext renderer_context)
{
	RendererContextVulkan* vulkan_context = (RendererContextVulkan*)renderer_context;

	vkDeviceWaitIdle(vulkan_context->interface.device);

	renderer_destroy_buffer(vulkan_context->interface.device, vulkan_context->render.quad_vertex_buffer);
	renderer_destroy_buffer(vulkan_context->interface.device, vulkan_context->render.quad_index_buffer);

	//vertex
	renderer_destroy_buffer(vulkan_context->interface.device, vulkan_context->render.vertex_buffer);
	renderer_destroy_buffer(vulkan_context->interface.device, vulkan_context->render.index_buffer);

	//texture
	renderer_destroy_image(vulkan_context->interface.device, vulkan_context->render.texture);
	vkDestroyDescriptorPool(vulkan_context->interface.device, vulkan_context->descriptor_pool, NULL);
	vkDestroyDescriptorSetLayout(vulkan_context->interface.device, vulkan_context->descriptor_set_layout, NULL);

	//font
	vkUnmapMemory(vulkan_context->interface.device, vulkan_context->render.text_vertex_buffer.memory);
	renderer_destroy_buffer(vulkan_context->interface.device, vulkan_context->render.text_vertex_buffer);
	vkUnmapMemory(vulkan_context->interface.device, vulkan_context->render.text_index_buffer.memory);
	renderer_destroy_buffer(vulkan_context->interface.device, vulkan_context->render.text_index_buffer);

	renderer_destroy_image(vulkan_context->interface.device, vulkan_context->render.font);

	vkDestroyDescriptorSetLayout(vulkan_context->interface.device, vulkan_context->render.font_descriptor_set_layout, NULL);
	vkDestroyPipeline(vulkan_context->interface.device, vulkan_context->pipelines.text_pipeline, NULL);
	vkDestroyPipelineLayout(vulkan_context->interface.device, vulkan_context->pipelines.text_pipeline_layout, NULL);

	vkDestroyCommandPool(vulkan_context->interface.device, vulkan_context->transfer_command_pool, NULL);

	for (U4 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		vkDestroyCommandPool(vulkan_context->interface.device, vulkan_context->frames[i].command_pool, NULL);
		vkDestroySemaphore(vulkan_context->interface.device, vulkan_context->frames[i].semaphore_presentation_complete, NULL);
		vkDestroyFence(vulkan_context->interface.device, vulkan_context->frames[i].fence_draw_complete, NULL);
	}

	for (U4 i = 0; i < MAX_SWAPCHAIN_IMAGES; ++i)
	{
		vkDestroySemaphore(vulkan_context->interface.device, vulkan_context->semaphore_render_complete[i], NULL);
	}


	vkDestroyPipeline(vulkan_context->interface.device, vulkan_context->pipelines.image_pipeline, NULL);
	vkDestroyPipelineLayout(vulkan_context->interface.device, vulkan_context->pipelines.image_pipeline_layout, NULL);
	vkDestroyShaderModule(vulkan_context->interface.device, vulkan_context->pipelines.shader_module, NULL);


	renderer_cleanup_present(
		&vulkan_context->present, 
		vulkan_context->interface.device
	);
	renderer_cleanup_interface(&vulkan_context->interface);
	renderer_cleanup_platform(&vulkan_context->platform);
}

F4 renderer_measure_text(RendererContext renderer_context, StringView string_view)
{
	RendererContextVulkan* vulkan_context = (RendererContextVulkan*)renderer_context;
	F4 result = 0;
	for (U4 i = 0; i < string_view.size; ++i)
	{
		Char c = string_view.data[i];
		if (c < GLYPH_RANGE_START || c >= GLYPH_RANGE_END)
			continue;

		result += (F4)vulkan_context->render.font_metrics.glyphs[c - GLYPH_RANGE_START].advance;
	}
	return result;
}

F4 renderer_get_font_size(RendererContext renderer_context)
{
	RendererContextVulkan* vulkan_context = (RendererContextVulkan*)renderer_context;
	F4 result = (F4)vulkan_context->render.font_metrics.font_size;
	return result;
}
