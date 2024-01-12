#include "vk_engine.h"
#include "VkBootstrap.h"
#include "vk_log.h"
#include "io/vk_io.h"
#include "io/settings.h"
//#include <glm/gtx/transform.hpp>
#include "vk_texture.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

void VkEngine::init()
{

    initWindow();

    initVulkan();

    initSwapchain();

    initCommands();

    initRenderPass();

    initFramebuffers();

    initSync();

    initDescriptors();

    initPipelines();

    loadMeshes();

    loadImages();

    initScene();

    _init = true;
}

void VkEngine::run()
{
    while(!glfwWindowShouldClose(_window)){
        glfwPollEvents();
        draw();
    }
}

void VkEngine::draw()
{
    FrameData& frame = getCurrentFrame();

    //Wait for fence signal (when gpu finishes work)
    VK_CHECK(vkWaitForFences(_device,1,&frame._renderDone,true,1000000000));
    //Reset fence so it can be signaled
    VK_CHECK(vkResetFences(_device,1,&frame._renderDone));


    uint32_t currentImg;
    VK_CHECK(vkAcquireNextImageKHR(_device,_swapchain,1000000000,frame._imgReadyFlag,VK_NULL_HANDLE,&currentImg));

    //Reset command buffer
    VK_CHECK(vkResetCommandBuffer(frame._commandBuffer,0));

    VkCommandBuffer cmd = frame._commandBuffer;

    VkCommandBufferBeginInfo cmd_info{};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;   
    cmd_info.pNext = nullptr;

    //Used for secondary buffers
    cmd_info.pInheritanceInfo = nullptr;
    //Let driver know we're only submitting this buffer once
    cmd_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    //Start recording
    VK_CHECK(vkBeginCommandBuffer(cmd,&cmd_info));

    //Specify render pass info

    //Color clear value
    VkClearValue clear_value{};
    float flash = abs(sin(_frameNum / 120.0f));
    clear_value.color = {{0.0,0.0,flash,1.0f}};

    //Depth clear value
    VkClearValue depth_value{};
    //Clear all to 1, smaller than 1 is "closer"
    depth_value.depthStencil.depth = 1.f; 

    VkClearValue clear_values[2] = { clear_value,depth_value };

    VkRenderPassBeginInfo pass_info{};
    pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_info.pNext = nullptr;

    //Info for starting render pass
    pass_info.renderPass = _renderPass;
    pass_info.renderArea.offset.x = 0;
    pass_info.renderArea.offset.y = 0;
    pass_info.renderArea.extent = _extent;
    pass_info.framebuffer = _framebuffers[currentImg];

    pass_info.clearValueCount = 2;
    pass_info.pClearValues = &clear_values[0];

    vkCmdBeginRenderPass(cmd,&pass_info,VK_SUBPASS_CONTENTS_INLINE);

    //HERE is where render resource binding and drawing commands happen!

    //Bind pipeline
    vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,_meshPipeline);

    //Update descriptor set (usually based on material)

    float val = _frameNum / 120.0f;

    _sceneParams.ambientColor = { sin(val),0,cos(val),1 };

    //Update scene param buffer based on current frame
    uint32_t frameIdx = _frameNum % NUM_FRAMES;
    uint32_t uniformOffset = vk_init::padBufferSize(_gpuProperties.limits.minUniformBufferOffsetAlignment, sizeof(GPUSceneData)) * frameIdx;


    char* sceneData;
    vmaMapMemory(_allocator, _sceneParamBuffer._allocation, (void**)& sceneData);

    //Set appropriate buffer offset
    sceneData += uniformOffset;
    
    memcpy(sceneData, &_sceneParams, sizeof(GPUSceneData));

    vmaUnmapMemory(_allocator, _sceneParamBuffer._allocation);



   //Bind correct descriptor set to attach to pipeline
   //Using dynamic descriptors per frame
    //Note: if you pass multiple uniform offset, in order the bind will adjust the offset of all DYNAMIC descriptors in the set
 
    //Bind set 0
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 0, 1, &frame._globalDescriptor, 1,&uniformOffset);
    //Bind set 1
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 1, 1, &frame._objectDescriptorSet,0,nullptr);
    //Bind set 2
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 2, 1, &_textureSet, 0, nullptr);


    math::Vec3 camOrigin = { 0,0,-2 };

    math::Vec3 center = { 10 * cos(val),4,0};

    math::Mat4 view = math::Mat4::lookAt(math::Vec3{0, 4, -(_gridSize+1)*0.5f*_gridSpacing}, center, math::Vec3{0, 1, 0});


    math::Mat4 proj = math::Mat4::perspectiveProjection(70.0 * (3.14 / 180.0), (float)_extent.width / (float)_extent.height, 0.1f, 200.0f);
    //Flip y axis
    proj[1][1] *= -1;


    //Actually write camera data to uniform buffer
    GPUCameraData cameraData{};
    cameraData.proj = proj;
    cameraData.view = view;
    cameraData.viewproj = proj * view;

    //Do CPU -> GPU copy
    void* data;
    vmaMapMemory(_allocator, frame._cameraBuffer._allocation, &data);

    memcpy(data, &cameraData, sizeof(GPUCameraData));

    vmaUnmapMemory(_allocator, frame._cameraBuffer._allocation);

    

    //Update object positions

    void* objData;
    vmaMapMemory(_allocator, frame._objectBuffer._allocation, &objData);

    uint32_t objId = 0;
    GPUObjectData* currentObjData = (GPUObjectData*)objData;


    int half_grid_size = _gridSize / 2;
    for (int i = -half_grid_size; i < half_grid_size; i++) {
        for (int j = -half_grid_size; j < half_grid_size; j++) {
            math::Mat4 model = math::Mat4::fromRotateYAxis(_frameNum * 0.4f * 0.01);
            model = math::Mat4::fromTranslation(math::Vec3{i* _gridSpacing, 0.0, j* _gridSpacing})* model;

            currentObjData->modelMatrix = model;
            currentObjData++;
            objId++;
        }
    }

    //For monkey, just use identity

    //currentObjData->modelMatrix = math::Mat4::fromTranslation(0, 4, 0) * math::Mat4::fromRotateYAxis(_frameNum * 0.4f * 0.01);
    currentObjData->modelMatrix = math::Mat4::fromTranslation(5, -10, 0);

    vmaUnmapMemory(_allocator, frame._objectBuffer._allocation);

    uint32_t totalNumObjects = objId;


    //Draw triangles first
    //Bind vertex input buffers
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &_triMesh._vertexBuffer._buffer, &offset);

    for (uint32_t i = 0; i < totalNumObjects; i++) {
        //Omit this draw now
        //vkCmdDraw(cmd, _triMesh._vertices.size(), 1, 0, i);
    }

    //Draw monkey mesh
    vkCmdBindVertexBuffers(cmd, 0, 1, &_triMesh._vertexBuffer._buffer, &offset);

    vkCmdDraw(cmd, _triMesh._vertices.size(), 1, 0, totalNumObjects);
   

    vkCmdEndRenderPass(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    //Now we submit the recorded command

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    //ADVANCED SYNC
    submit_info.pWaitDstStageMask = &wait_stage;

    //Wait for swapchain to have img ready
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &frame._imgReadyFlag;

    //Signal render done flag when done
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &frame._renderDoneFlag;

    //Submit single recorded command buffer
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    VK_CHECK(vkQueueSubmit(_graphicsQueue,1,&submit_info,frame._renderDone));

    //Finally, have to give final image to swapchain

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &frame._renderDoneFlag;

    present_info.pImageIndices = &currentImg;

    VK_CHECK(vkQueuePresentKHR(_graphicsQueue,&present_info));

    _frameNum++;
}

void VkEngine::cleanup()
{
    if(_init){

        for (int i = 0; i < NUM_FRAMES; i++) {
            //Wait for last gpu command to finish
            VK_CHECK(vkWaitForFences(_device, 1, &_frames[i]._renderDone, true, 1000000000));
        }

        //Destroy samplers
        vkDestroySampler(_device, _blockSampler, nullptr);

        //Destroy images

        vkDestroyImageView(_device, _depthImageView, nullptr);
        vkDestroyImageView(_device, _mainTexture.imageView, nullptr);

        vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
        vmaDestroyImage(_allocator, _mainTexture.image._image, _mainTexture.image._allocation);

        //Destroy vertex buffers
        //vmaDestroyBuffer(_allocator, _monkeyMesh._vertexBuffer._buffer, _monkeyMesh._vertexBuffer._allocation);
        vmaDestroyBuffer(_allocator, _triMesh._vertexBuffer._buffer, _triMesh._vertexBuffer._allocation);

        //Destroy uniform buffers
        for (int i = 0; i < NUM_FRAMES; i++) {
            vmaDestroyBuffer(_allocator, _frames[i]._cameraBuffer._buffer, _frames[i]._cameraBuffer._allocation);
            vmaDestroyBuffer(_allocator, _frames[i]._objectBuffer._buffer, _frames[i]._objectBuffer._allocation);
        }

        vmaDestroyBuffer(_allocator, _sceneParamBuffer._buffer, _sceneParamBuffer._allocation);

        vmaDestroyAllocator(_allocator);

        //Destroy descriptor set layouts
        vkDestroyDescriptorSetLayout(_device, _globalSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _objectSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(_device, _textureSetLayout, nullptr);
        vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);

        vkDestroyPipeline(_device,_meshPipeline,nullptr);

        vkDestroyPipelineLayout(_device,_meshPipelineLayout,nullptr);

        //Sync destroy

        vkDestroyFence(_device, _uploadContext._uploadFence, nullptr);

        for (int i = 0; i < NUM_FRAMES; i++) {
            vkDestroyFence(_device, _frames[i]._renderDone, nullptr);
            vkDestroySemaphore(_device, _frames[i]._imgReadyFlag, nullptr);
            vkDestroySemaphore(_device, _frames[i]._renderDoneFlag, nullptr);
        }

        


        vkDestroyRenderPass(_device,_renderPass,nullptr);

        //Destroy command pools

        vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr);

        for (int i = 0; i < NUM_FRAMES; i++) {
            vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
        }

        for(uint32_t i =0;i< _swapchainImageViews.size();i++){
            vkDestroyFramebuffer(_device,_framebuffers[i],nullptr);
            vkDestroyImageView(_device,_swapchainImageViews[i],nullptr);
        }


        vkDestroySwapchainKHR(_device,_swapchain,nullptr);

        vkDestroyDevice(_device,nullptr);

        vkDestroySurfaceKHR(_instance,_surface,nullptr);

        vkb::destroy_debug_utils_messenger(_instance,_debugMessenger,nullptr);
        vkDestroyInstance(_instance,nullptr);

    }
}

void VkEngine::initWindow()
{
    glfwInit();

    //No opengl
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(_extent.width, _extent.height, "learnVk", nullptr, nullptr);

    int width, height;

    glfwGetFramebufferSize(_window, &width, &height);

}

void VkEngine::initVulkan()
{
    //Create instance
    vkb::InstanceBuilder builder{};

    auto res = builder
        .set_app_name("learnVk")
        .request_validation_layers(true)
        .require_api_version(1,1,0)
        .use_default_debug_messenger()
        .build();

    vkb::Instance instance = res.value();

    _instance = instance.instance;
    _debugMessenger = instance.debug_messenger;

    //Create surface

    VK_CHECK(glfwCreateWindowSurface(_instance,_window,nullptr,&_surface));

    //Create device

    vkb::PhysicalDeviceSelector selector{instance};
    vkb::PhysicalDevice gpu = selector
        .set_minimum_version(1,1)
        .set_surface(_surface)
        .select()
        .value();

    vkb::DeviceBuilder deviceBuilder{gpu};

    VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_parameters_features{};
    shader_draw_parameters_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    shader_draw_parameters_features.pNext = nullptr;
    shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;

    vkb::Device device = deviceBuilder
        .add_pNext(&shader_draw_parameters_features)
        .build()
        .value();

    _gpuProperties = device.physical_device.properties;

    std::cout << "GPU min alignment: " << _gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;

    _gpu = gpu;
    _device = device.device;

    //Get queue handles

    _graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();
    _graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();

    //Create memory allocator

    VmaAllocatorCreateInfo create_info{};
    create_info.instance = _instance;
    create_info.physicalDevice = _gpu;
    create_info.device = _device;

    VK_CHECK(vmaCreateAllocator(&create_info, &_allocator));
}

void VkEngine::initSwapchain()
{

    vkb::SwapchainBuilder builder{_gpu,_device,_surface};

    vkb::Swapchain swapchain = builder
        .use_default_format_selection()
        .set_desired_extent(_extent.width,_extent.height)
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .build()
        .value();


    _extent.width = swapchain.extent.width;
    _extent.height = swapchain.extent.height;


    _swapchain = swapchain.swapchain;
    _swapchainFormat = swapchain.image_format;
    _swapchainImages = swapchain.get_images().value();
    _swapchainImageViews = swapchain.get_image_views().value();

    //Create depth image
    VkExtent3D depthImgExtent{
        _extent.width,
        _extent.height,
        1
    };

    _depthFormat = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo img_info = vk_init::imageCreateInfo(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImgExtent);

    VmaAllocationCreateInfo alloc_info{};
    //For speed, want gpu access to image only (this is a suggestion to driver)
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    //This is a requirement to the driver to allocate image in VRAM
    alloc_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    //Allocate image
    vmaCreateImage(_allocator, &img_info, &alloc_info, &_depthImage._image, &_depthImage._allocation, nullptr);

    //Depth bit again lets driver know is being used for depth testing
    VkImageViewCreateInfo img_view_info = vk_init::imageViewCreateInfo(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(_device,&img_view_info,nullptr,&_depthImageView));
}

void VkEngine::initCommands()
{
    for (int i = 0; i < NUM_FRAMES; i++) {

        //Create command pool
        VkCommandPoolCreateInfo create_info = vk_init::commandPoolCreateInfo(_graphicsQueueFamily);
        VK_CHECK(vkCreateCommandPool(_device, &create_info, nullptr, &_frames[i]._commandPool));

        //Create command buffer
        VkCommandBufferAllocateInfo alloc_info = vk_init::commandBufferAllocateInfo(_frames[i]._commandPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(_device, &alloc_info, &_frames[i]._commandBuffer));

    }


    VkCommandPoolCreateInfo upload_info = vk_init::commandPoolCreateInfo(_graphicsQueueFamily);
    VK_CHECK(vkCreateCommandPool(_device, &upload_info, nullptr, &_uploadContext._commandPool));

    VkCommandBufferAllocateInfo alloc_info = vk_init::commandBufferAllocateInfo(_uploadContext._commandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(_device,&alloc_info,&_uploadContext._commandBuffer));
}

void VkEngine::initRenderPass()
{
    /* Attachments */

    //Color attachment
    VkAttachmentDescription color_attachment{};

    //Use swapchain format
    color_attachment.format = _swapchainFormat;
    //1 sample per pixel
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    //Clear on load
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //Keep attachment after render pass
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    
    //Stencil ops = don't care
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    //Init = don't care about layout
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //Final = make sure swapchain is ready for it
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    //Depth attachment
    VkAttachmentDescription depth_attachment{};

    //Use swapchain format
    depth_attachment.format = _depthFormat;
    //1 sample per pixel
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    //Clear on load
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //Keep attachment after render pass
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    //Stencil ops = don't care
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    //Init = don't care about layout
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    //Final = make sure swapchain is ready for it
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    /* Subpasses */

    VkAttachmentReference color_attachment_ref{};

    //Index into attachment array
    color_attachment_ref.attachment = 0;
    //Whatever is best for the driver
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};

    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;
   

    VkAttachmentDescription attachments[2] = { color_attachment,depth_attachment };


    /* Subpass dependencies */

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depth_dependency{};
    depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depth_dependency.dstSubpass = 0;
    depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.srcAccessMask = 0;
    depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependencies[2] = { dependency,depth_dependency };

    /* Render pass */

    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.pNext = nullptr;

    //Attachments
    create_info.attachmentCount = 2;
    create_info.pAttachments = &attachments[0];

    //Subpasses
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass;

    //Subpass dependencies
    create_info.dependencyCount = 2;
    create_info.pDependencies = &dependencies[0];

    VK_CHECK(vkCreateRenderPass(_device,&create_info,nullptr,&_renderPass));

}


void VkEngine::initFramebuffers()
{
    VkFramebufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.pNext = nullptr;

    create_info.renderPass = _renderPass;
    create_info.attachmentCount = 1;
    create_info.width = _extent.width;
    create_info.height = _extent.height;
    create_info.layers = 1;

    const uint32_t numImages = _swapchainImageViews.size();
    _framebuffers.resize(numImages);

    for(uint32_t i = 0; i < numImages;i++){

        VkImageView attachments[2] = { _swapchainImageViews[i],_depthImageView };

        create_info.attachmentCount = 2;
        create_info.pAttachments = &attachments[0];

        VK_CHECK(vkCreateFramebuffer(_device,&create_info,nullptr,&_framebuffers[i]));
    }
}

void VkEngine::initSync()
{

    //Init fence for data transfer

    VkFenceCreateInfo upload_fence_info{};
    upload_fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    upload_fence_info.pNext = nullptr;

    //Don't start it signaled
    upload_fence_info.flags = 0;

    VK_CHECK(vkCreateFence(_device, &upload_fence_info, nullptr, &_uploadContext._uploadFence));


    

    for (int i = 0; i < NUM_FRAMES; i++) {

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.pNext = nullptr;

        //Start fence in signaled state
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_CHECK(vkCreateFence(_device, &fence_info, nullptr, &_frames[i]._renderDone));

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = nullptr;

        //None
        semaphore_info.flags = 0;

        VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr, &_frames[i]._imgReadyFlag));
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr, &_frames[i]._renderDoneFlag));

    }
}

void VkEngine::initDescriptors()
{
  

    //Create descriptor set layouts

    //Here we describe how the descriptor set via all its bindings

    //Descriptor set 0

    //Set 0,Binding 0
    VkDescriptorSetLayoutBinding camera_buffer_binding = vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
   
    //Set 0,Binding 1
    VkDescriptorSetLayoutBinding scene_param_binding = vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);

    VkDescriptorSetLayoutBinding bindings[] = { camera_buffer_binding,scene_param_binding };

    //Create actual layouts for descriptor sets

    VkDescriptorSetLayoutCreateInfo layout_create_info{};
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.pNext = nullptr;

    //2 Bindings
    layout_create_info.bindingCount = 2;
    //no flags
    layout_create_info.flags = 0;
    //Description of all bindings we're gonna attach
    layout_create_info.pBindings = bindings;

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &layout_create_info, nullptr, &_globalSetLayout));

    //Descriptor set 1

    //Set 1,Binding 0
    VkDescriptorSetLayoutBinding object_binding = vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

    VkDescriptorSetLayoutCreateInfo layout1_create_info{};
    layout1_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout1_create_info.pNext = nullptr;

    //1 storage buffer binding
    layout1_create_info.bindingCount = 1;
    //no flags
    layout1_create_info.flags = 0;
    //Description of all bindings we're gonna attach
    layout1_create_info.pBindings = &object_binding;

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &layout1_create_info, nullptr, &_objectSetLayout));

    //Descriptor set 2
    
    //Set 2,Binding 0
    VkDescriptorSetLayoutBinding tex_binding = vk_init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

    VkDescriptorSetLayoutCreateInfo layout2_create_info{};
    layout2_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout2_create_info.pNext = nullptr;

    //1 storage buffer binding
    layout2_create_info.bindingCount = 1;
    //no flags
    layout2_create_info.flags = 0;
    //Description of all bindings we're gonna attach
    layout2_create_info.pBindings = &tex_binding;

    VK_CHECK(vkCreateDescriptorSetLayout(_device, &layout2_create_info, nullptr, &_textureSetLayout));


    //Allocate descriptor POOL

    //This structure describes what types of handles the pool can hand out
    std::vector<VkDescriptorPoolSize> sizes = 
    {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,10}, //Max # uniform buffer descriptors to allocate
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 }, //Is basically a moveable pointer inside the uniform buffer
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 }, //Storage buffer descriptors
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
    };

    VkDescriptorPoolCreateInfo pool_create_info{};
    pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    //none
    pool_create_info.flags = 0;
    //Max # sets to allocate
    pool_create_info.maxSets = 10;
    pool_create_info.poolSizeCount = (size_t)sizes.size();
    pool_create_info.pPoolSizes = sizes.data();

    VK_CHECK(vkCreateDescriptorPool(_device,&pool_create_info,nullptr,&_descriptorPool));

    //Allocate the "environment buffer"
    size_t minAlignmentSize = _gpuProperties.limits.minUniformBufferOffsetAlignment;
    size_t sceneBufferSize = NUM_FRAMES * vk_init::padBufferSize(minAlignmentSize, sizeof(GPUSceneData));

    //Scene buffer
    vk_init::createBuffer(sceneBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, _allocator, &_sceneParamBuffer);

    //First create per frame uniform buffers for camera data
    //Then allocate a descriptor set (with a single uniform buffer binding) for each of these
    //Needs to be per frame since camera might change per frame
    for (int i = 0; i < NUM_FRAMES; i++) {

        //Allocate camera uniform buffer (per frame)
        vk_init::createBuffer(sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, _allocator, &_frames[i]._cameraBuffer);

        //Allocate object buffer for each frame
        vk_init::createBuffer(sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,_allocator,&_frames[i]._objectBuffer);

        //Allocates actual descriptor set from pool (with single binding)
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = nullptr;

        //Use the pool we previously made
        alloc_info.descriptorPool = _descriptorPool;

        //Just ask for 1 descriptor set
        alloc_info.descriptorSetCount = 1;
        //Use the corresponding layouts
        alloc_info.pSetLayouts = &_globalSetLayout;

        VK_CHECK(vkAllocateDescriptorSets(_device, &alloc_info,&_frames[i]._globalDescriptor));

        //Allocates descriptor set for objects
        VkDescriptorSetAllocateInfo obj_alloc_info{};
        obj_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        obj_alloc_info.pNext = nullptr;

        //Use the pool we previously made
        obj_alloc_info.descriptorPool = _descriptorPool;

        //Just ask for 1 descriptor set
        obj_alloc_info.descriptorSetCount = 1;
        //Use the corresponding layouts
        obj_alloc_info.pSetLayouts = &_objectSetLayout;

        VK_CHECK(vkAllocateDescriptorSets(_device, &obj_alloc_info, &_frames[i]._objectDescriptorSet));

        //Get binding of descriptor set to point to the actual camera uniform buffer
        //Note this is for updating a SINGLE binding within a descriptor set
        //You would need multiple of these to update the whole set if it had many bindings
        VkDescriptorBufferInfo buffer_info{};
        //Which buffer to point to?
        buffer_info.buffer = _frames[i]._cameraBuffer._buffer;
        //Offset into buffer
        buffer_info.offset = 0;
        //Size of data to read
        buffer_info.range = sizeof(GPUCameraData);

        //Each frame pts to a diff part of the same buffer (function of index i)
        VkDescriptorBufferInfo params_info{};
        //Which buffer to point to?
        params_info.buffer = _sceneParamBuffer._buffer;
        //Offset into buffer
        //params_info.offset = vk_init::padBufferSize(minAlignmentSize, sizeof(GPUSceneData)) * i;
        //Gonna use dynamic offset at draw time 
        params_info.offset = 0; 
        //Size of data to read
        params_info.range = sizeof(GPUSceneData);


        //Buffer binding for object daat
        VkDescriptorBufferInfo obj_buffer_info{};
        //Which buffer to point to?
        obj_buffer_info.buffer = _frames[i]._objectBuffer._buffer;
        //Offset into buffer
        //params_info.offset = vk_init::padBufferSize(minAlignmentSize, sizeof(GPUSceneData)) * i;
        //Gonna use dynamic offset at draw time 
        obj_buffer_info.offset = 0;
        //Size of data to read
        obj_buffer_info.range = sizeof(GPUObjectData) * MAX_OBJECTS;

        //Actual writing data per binding

        //Prepare to attach camera buffer to set 0,binding 0
        VkWriteDescriptorSet write_set_camera = vk_init::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _frames[i]._globalDescriptor, &buffer_info, 0);
    
        //Prepare to attach scene params to set 0,binding 0
        VkWriteDescriptorSet write_set_scene = vk_init::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, _frames[i]._globalDescriptor, &params_info, 1);

        //Prepare to attache object params to set 1,binding 0
        VkWriteDescriptorSet write_set_obj = vk_init::writeDescriptorBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _frames[i]._objectDescriptorSet, &obj_buffer_info, 0);

        VkWriteDescriptorSet writes[] = {write_set_camera,write_set_scene,write_set_obj};

        //NOTE: This call can execute multiple writes (for diff descriptors in a set/or even acrros diff sets at once)
        vkUpdateDescriptorSets(_device, 3, writes, 0, nullptr);
    }

   

}

void VkEngine::initPipelines()
{

    /* Shader Modules */
    std::string meshVertPath = SHADER_DIR + std::string{"tex_mesh.vert.spv"};
    std::string meshFragPath = SHADER_DIR + std::string{"tex_mesh.frag.spv"};

    VkShaderModule meshVertShader;
    VkShaderModule meshFragShader;

    if(!vk_io::loadShaderModule(_device,meshVertPath.c_str(),&meshVertShader)){
        std::cerr << "Couldn't load vertex shader: " << meshVertPath.c_str() << std::endl;
    }else{
        std::cout << "Loaded vertex shader: " <<  meshVertPath.c_str() << std::endl;
    }

    if(!vk_io::loadShaderModule(_device,meshFragPath.c_str(),&meshFragShader)){
        std::cerr << "Couldn't load fragment shader: " << meshFragPath.c_str() << std::endl;
    }else{
        std::cout << "Loaded fragment shader: " <<  meshFragPath.c_str() << std::endl;
    }

    /* Pipeline layout */
    
    VkPipelineLayoutCreateInfo layout_info = vk_init::pipelineLayoutCreateInfo();

    VkPushConstantRange push_constant{};
    //Offset into mem (where shader reads from)
    push_constant.offset = 0;
    //Size (where shader reads from)
    push_constant.size = sizeof(MeshPushConstants);
    //Accessible in vertex shader
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    //Push constant ranges
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push_constant;

    //Descriptor set layouts

    VkDescriptorSetLayout layouts[] = { _globalSetLayout,_objectSetLayout,_textureSetLayout };

    layout_info.setLayoutCount = 3;
    layout_info.pSetLayouts = layouts;

    VK_CHECK(vkCreatePipelineLayout(_device,&layout_info,nullptr,&_meshPipelineLayout));

    /* Pipeline configuration */

    vk_init::PipelineBuilder builder{};

    builder._shaderStages.emplace_back(
        vk_init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,meshVertShader)
    );

    builder._shaderStages.emplace_back(
        vk_init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT,meshFragShader)
    );

    /* Vertex input info */

    builder._vertexInputInfo = vk_init::pipelineVertexInputStateCreateInfo();

    VertexInputDescription desc = Vertex::getVertexDescription();

    builder._vertexInputInfo.vertexBindingDescriptionCount = desc.bindings.size();
    builder._vertexInputInfo.pVertexBindingDescriptions = desc.bindings.data();

    builder._vertexInputInfo.vertexAttributeDescriptionCount = desc.attributes.size();
    builder._vertexInputInfo.pVertexAttributeDescriptions = desc.attributes.data();
   

    builder._inputAssembly = vk_init::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);


    builder._viewport.x = 0.0f;
    builder._viewport.y = 0.0f;
    builder._viewport.width = (float)_extent.width;
    builder._viewport.height = (float)_extent.height;
    builder._viewport.minDepth = 0.0f;
    builder._viewport.maxDepth = 1.0f;

    builder._scissor.offset = {0,0};
    builder._scissor.extent = _extent;


    builder._rasterizer = vk_init::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

    builder._depthStencil = vk_init::pipelineDepthStencilStateCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    builder._colorBlendAttachment = vk_init::pipelineColorBlendAttachmentState();

    builder._multisampling = vk_init::pipelineMultisampleStateCreateInfo();

    builder._pipelineLayout = _meshPipelineLayout;

    _meshPipeline = builder.buildPipeline(_device,_renderPass);


    //Cleanup modules
    vkDestroyShaderModule(_device,meshVertShader,nullptr);
    vkDestroyShaderModule(_device,meshFragShader,nullptr);
}

void VkEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VkCommandBuffer cmd = _uploadContext._commandBuffer;

    VkCommandBufferBeginInfo begin_info = vk_init::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = vk_init::submitInfo(&cmd);
    
    VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence));

    vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
    vkResetFences(_device, 1, &_uploadContext._uploadFence);

    vkResetCommandPool(_device, _uploadContext._commandPool, 0);

}

void VkEngine::loadMeshes()
{

   

    std::string mesh_path = MODEL_DIR + std::string{"lost_empire.obj"};   

    vk_io::loadMeshFromFile(mesh_path.c_str(), _triMesh);

    /*_triMesh._vertices.resize(3);

    _triMesh._vertices[0].position = { 1,1,0 };
    _triMesh._vertices[1].position = { -1,1,0 };
    _triMesh._vertices[2].position = { 0,-1,0 };

    _triMesh._vertices[0].color = { 0,1,0 };
    _triMesh._vertices[1].color = { 0,1,0 };
    _triMesh._vertices[2].color = { 0,1,0 };

    */

    uploadMesh(_triMesh);
    //uploadMesh(_monkeyMesh);
}

void VkEngine::loadImages()
{
    std::string imgPath = IMAGE_DIR + std::string{"lost_empire-RGBA.png"};

    loadImgFromFile(*this, imgPath.c_str(), _mainTexture.image);

    VkImageViewCreateInfo view_info = vk_init::imageViewCreateInfo(VK_FORMAT_R8G8B8A8_SRGB, _mainTexture.image._image, VK_IMAGE_ASPECT_COLOR_BIT);

    VK_CHECK(vkCreateImageView(_device,&view_info,nullptr,&_mainTexture.imageView));


}

void VkEngine::initScene()
{
    //Create a sampler for texture
    //NOTE: Knows nothing of the actual image or image view!

    VkSamplerCreateInfo sampler_info = vk_init::samplerCreateInfo(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);

    VK_CHECK(vkCreateSampler(_device, &sampler_info, nullptr, &_blockSampler));

    //Allocate descriptor set for textures
    VkDescriptorSetAllocateInfo des_set_alloc{};
    des_set_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    des_set_alloc.pNext = nullptr;

    //Use the pool we previously made
    des_set_alloc.descriptorPool = _descriptorPool;

    //Just ask for 1 descriptor set
    des_set_alloc.descriptorSetCount = 1;
    //Use the corresponding layouts
    des_set_alloc.pSetLayouts = &_textureSetLayout;

    VK_CHECK(vkAllocateDescriptorSets(_device, &des_set_alloc, &_textureSet));

    //Where sampler and image view get combined
    VkDescriptorImageInfo img_info{};
    img_info.sampler = _blockSampler;
    img_info.imageView = _mainTexture.imageView;
    img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

   

    //Make write to descriptor set
    VkWriteDescriptorSet write_set_obj = vk_init::writeDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _textureSet, &img_info, 0);


    //NOTE: This call can execute multiple writes (for diff descriptors in a set/or even acrros diff sets at once)
    vkUpdateDescriptorSets(_device, 1, &write_set_obj, 0, nullptr);
}

void VkEngine::uploadMesh(Mesh& mesh)
{

    size_t buffer_size = mesh._vertices.size() * sizeof(Vertex);

    //First create CPU side buffer
    AllocatedBuffer stagingBuffer{};
    vk_init::createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,_allocator, &stagingBuffer);

    //Copy data to cpu side buffer
    void* data;
    vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
    memcpy(data, mesh._vertices.data(),buffer_size);
    vmaUnmapMemory(_allocator, stagingBuffer._allocation);

    
    //Create GPU side buffer
    vk_init::createBuffer(mesh._vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, _allocator,&mesh._vertexBuffer);

    immediate_submit([=](VkCommandBuffer cmd) {
        VkBufferCopy copy{};
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        copy.size = buffer_size;
        vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._vertexBuffer._buffer, 1, &copy);
    });

    //Can delete staging buffer now
    vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

FrameData& VkEngine::getCurrentFrame()
{
    return _frames[_frameNum % NUM_FRAMES];
}
